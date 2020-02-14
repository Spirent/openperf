package fsm

import (
	"context"
	"fmt"
	"net/url"
	"sync/atomic"
	"time"

	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
	"github.com/Spirent/openperf/targets/spiperf/openperf/v1/models"
	"github.com/go-openapi/strfmt/conv"
	"github.com/kr/pretty"
)

const FSMVersion = "1.0"

// Client is the FSM for the client stuff in spiperf
type Client struct {
	// PeerCmdOut sends commands to the server Spiperf instance.
	PeerCmdOut chan *msg.Message

	// PeerRespIn receives responses from the server Spiperf instance.
	PeerRespIn chan *msg.Message

	// PeerNotifIn receives notifications from the server Spiperf instance.
	PeerNotifIn chan *msg.Message

	// OpenperfCmdOut channel to send commands to Openperf Controller.
	OpenperfCmdOut chan *openperf.Command

	// PeerTimeout specifies the maximum time the FSM  will wait for responses from the peer instance.
	PeerTimeout time.Duration

	// OpenperfTimeout specifies the maximum time the FSM will wait for responses from the Openperf instance.
	OpenperfTimeout time.Duration

	// StartDelay how much extra time to add to the requested start time so server and client start
	// traffic generation and analysis at the same time.
	StartDelay time.Duration

	// TestConfiguration keeps track of the test configuration.
	TestConfiguration Configuration

	// state tracks the state machine's current state as a string.
	state atomic.Value

	// startTime is the time at which traffic will begin transmitting.
	startTime time.Time

	// opRunstatePollReq keeps track of traffic runstate between FSM states.
	opRunstatePollReq *openperf.CommandRepeater

	// opStatsPollReq keeps track of traffic result polling.
	opStatsPollReq *openperf.CommandRepeater

	statsReqCancel context.CancelFunc

	runstateReqCancel context.CancelFunc
}

func (c *Client) enter(state string) {
	c.state.Store(state)
	fmt.Printf("ENTER - %s\n", state)
}

func (c *Client) State() (state string) {
	state, _ = c.state.Load().(string)
	return
}

type clientStateFunc func(*Client, context.Context) (clientStateFunc, error)

func (c *Client) Run(ctx context.Context) error {

	if c.PeerCmdOut == nil {
		return &InternalError{Message: "Invalid Command Out Channel"}
	}
	if c.PeerRespIn == nil {
		return &InternalError{Message: "Invalid Response In Channel"}
	}
	if c.PeerNotifIn == nil {
		return &InternalError{Message: "Invalid Notification In Channel"}
	}
	if c.PeerTimeout <= 0 {
		return &InternalError{Message: "Invalid Peer Timeout"}
	}
	if c.OpenperfTimeout <= 0 {
		return &InternalError{Message: "Invalid Openperf Timeout"}
	}
	// if c.TestConfiguration == nil {
	// 	return &InternalError{Message: "Invalid Test Configuration"}
	// }

	defer func() {
		close(c.PeerCmdOut)
	}()

	f := (*Client).connect
	for f != nil {
		var err error
		if f, err = f(c, ctx); err != nil {
			return err
		}
	}

	return nil
}

func (c *Client) connect(ctx context.Context) (clientStateFunc, error) {
	c.enter("connect")

	// Send GetVersion message to remote spiperf
	c.PeerCmdOut <- &msg.Message{
		Type: msg.HelloType,
		Value: &msg.Hello{
			Version: FSMVersion,
		},
	}

	// Wait for reply
	timeout := time.After(c.PeerTimeout)
	select {
	//FIXME: overkill?
	case reply, ok := <-c.PeerRespIn:
		if !ok {
			return nil, &InternalError{Message: "Error reading from peer response channel"}
		}

		if reply.Type == msg.ErrorType {
			return nil, &InternalError{Message: reply.Value.(string)}
		}

		// Verify remote version matches our version.
		if serverVersion := reply.Value.(*msg.Hello).Version; serverVersion != FSMVersion {
			return nil, &InternalError{Message: fmt.Sprintf("Mismatch between server and client versions. Client version is %s and server reports as %s\n", FSMVersion, serverVersion)}
		}

	case <-timeout:
		return nil, &TimeoutError{Message: "Timed out waiting for server reply to Hello message."}
	}

	fmt.Println("EXIT - connect")
	return (*Client).configure, nil

}

func (c *Client) configure(ctx context.Context) (clientStateFunc, error) {
	c.enter("configure")

	// Send GetConfig to server.
	c.PeerCmdOut <- &msg.Message{
		Type: msg.GetConfigType,
	}

	// Wait for response.
	timeout := time.After(c.PeerTimeout)
	select {
	case reply, ok := <-c.PeerRespIn:
		if !ok {
			return nil, &InternalError{Message: "Error reading from peer response channel"}
		}

		if reply.Type == msg.ErrorType {
			return nil, &InternalError{Message: reply.Value.(string)}
		}

		serverPrams := reply.Value.(*msg.ServerParametersResponse)
		//FIXME: this just for now to get the unit tests off the ground. Full validation will happen later.
		if _, err := url.Parse(serverPrams.OpenperfURL); err != nil {
			return nil, &InvalidConfigurationError{Message: fmt.Sprintf("Got invalid OpenPerf URL from server: %s", serverPrams.OpenperfURL)}
		}

	case <-timeout:
		return nil, &TimeoutError{Message: "Timed out waiting for server reply to get parameters message."}
	}
	// FIXME: Build test configuration.

	// Send configuration to server.
	c.PeerCmdOut <- &msg.Message{
		Type: msg.SetConfigType,
		Value: &msg.ServerConfiguration{
			TransmitDuration: 10,
			FixedFrameSize:   128,
		},
	}

	// Wait for an ACK.
	reply := <-c.PeerRespIn

	if reply.Type != msg.AckType {
		return nil, &InvalidConfigurationError{Message: reply.Value.(string)}
	}

	// Switch to Ready state.

	fmt.Println("EXIT - configure")
	return (*Client).ready, nil
}

func (c *Client) ready(ctx context.Context) (clientStateFunc, error) {
	c.enter("ready")

	reqDone := make(chan struct{})
	opCtx, _ := context.WithTimeout(ctx, c.OpenperfTimeout)
	req := &openperf.Command{
		Request: &openperf.GetTimeRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	c.OpenperfCmdOut <- req

	// Wait for request to finish one way or another.
	<-reqDone
	//Results are done. Or it timed out.
	switch resp := req.Response.(type) {
	case *models.TimeKeeper:
		val := time.Time(conv.DateTimeValue(resp.Time))
		//calc start time
		c.startTime = val.Add(c.StartDelay)

		fmt.Printf("client sleeping for %s", time.Until(c.startTime).String())

		//send start time to server
		c.PeerCmdOut <- &msg.Message{
			Type:  msg.StartCommandType,
			Value: &msg.StartCommand{StartTime: c.startTime},
		}
	case error:
		return nil, &InternalError{Message: resp.Error()}
	}

	// wait for ack to start time message.
	reply := <-c.PeerRespIn

	if reply.Type != msg.AckType {
		return nil, &InvalidConfigurationError{Message: reply.Value.(string)}
	}

	fmt.Println("EXIT - ready")
	// switch to armed state
	return (*Client).armed, nil
}

// armed FSM state that waits for the start time to arrive before switching to running state.
func (c *Client) armed(ctx context.Context) (clientStateFunc, error) {
	c.enter("armed")

	// Wait for then to become now.
	time.Sleep(time.Until(c.startTime))

	var statsReqCtx context.Context
	statsReqCtx, c.statsReqCancel = context.WithCancel(ctx)
	c.opStatsPollReq = &openperf.CommandRepeater{
		Command:            &openperf.Command{Request: &openperf.GetStatsRequest{}},
		Interval:           time.Second * 1,
		OpenperfController: c.OpenperfCmdOut,
		Responses:          make(chan interface{})}
	go openperf.RunCommandRepeater(statsReqCtx, c.opStatsPollReq)

	fmt.Println("EXIT - armed")
	// Change to correct running state.

	switch {
	case c.TestConfiguration.UpstreamRate > 0 && c.TestConfiguration.DownstreamRate > 0:

		var runstateReqCtx context.Context
		runstateReqCtx, c.runstateReqCancel = context.WithCancel(ctx)
		c.opRunstatePollReq = &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetGeneratorRequest{}},
			Interval:           time.Second * 1,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          make(chan interface{})}
		go openperf.RunCommandRepeater(runstateReqCtx, c.opRunstatePollReq)

		return (*Client).runningTxRx, nil
	case c.TestConfiguration.UpstreamRate > 0:

		var runstateReqCtx context.Context
		runstateReqCtx, c.runstateReqCancel = context.WithCancel(ctx)
		c.opRunstatePollReq = &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetGeneratorRequest{}},
			Interval:           time.Second * 1,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          make(chan interface{})}
		go openperf.RunCommandRepeater(runstateReqCtx, c.opRunstatePollReq)

		return (*Client).runningTx, nil
	case c.TestConfiguration.DownstreamRate > 0:
		return (*Client).runningRx, nil
	}

	return nil, &InternalError{Message: "upstream and downstream data rates are both zero."}
}

// runningRx tracks FSM state where client is only receiving traffic.
func (c *Client) runningRx(ctx context.Context) (clientStateFunc, error) {
	c.enter("runningRx")

	// Loop reading from notification channel. Until we see the stop notification from the server.
	//FIXME: make the timeout max(client_duration, server_duration) + fudge_factor
	timeout := time.After(20 * time.Second)
Done:
	for {
		select {
		case notif := <-c.PeerNotifIn: //FIXME: make comma-ok here. might get nil returned.
			switch notif.Type {
			case msg.ErrorType:
				//Write out error.
				return nil, &InternalError{Message: notif.Value.(string)}
			case msg.StopNotificationType:
				//Test is done.
				break Done
			case msg.StatsNotificationType:
				fmt.Println("got some stats from the server. should be only server tx stats.")

				//FIXME add a go routine to handle client side stats?
			}
		case <-timeout:
			return nil, &TimeoutError{Message: "Server did not send stop notification within expected time window."}
		}
	}

	fmt.Println("EXIT - runningRx")
	return (*Client).done, nil
}

// runningTx tracks FSM state where client is only sending traffic.
func (c *Client) runningTx(ctx context.Context) (clientStateFunc, error) {
	c.enter("runningTx")

Done:
	for {
		select {
		case notif, ok := <-c.PeerNotifIn:
			if !ok {
			}

			switch notif.Type {
			case msg.ErrorType:
				//Write out error.
				return nil, &InternalError{Message: notif.Value.(string)}
			case msg.StopNotificationType:
				//This is an error. Not expecting a stop here since server isn't transmitting.
				return nil, &MessagingError{Message: "Got unexpected server done notification"}
			case msg.StatsNotificationType:
				fmt.Println("got some stats from the server. should only be server rx stats.")
				//FIXME add a go routine to handle client side stats?
			}
		case stat, ok := <-c.opStatsPollReq.Responses:
			if !ok {
				//Something went wrong. Wait till command finishes and read Result.
				//<-c.opStatsPollReq.Responses
				//return nil, &InternalError{Message: c.opStatsPollReq.Result.(error).Error()}
			}
			pretty.Println(stat)
			//FIXME do something with the local tx stats.
		case gen, ok := <-c.opRunstatePollReq.Responses:
			if !ok {

			}

			generator, genOk := gen.(*openperf.GetGeneratorResponse)
			if !genOk {
				c.statsReqCancel()
				c.runstateReqCancel()
				return nil, &InternalError{Message: gen.(error).Error()}
			}

			if !generator.Running {
				fmt.Println("\t\tgenerator stopped running!")
				//Stop local stats polling.
				c.statsReqCancel()

				//Stop runstate polling.
				c.runstateReqCancel()

				lastStat, ok := <-c.opStatsPollReq.Responses
				if ok {
					//do something with lastStat
					fmt.Printf("%T", lastStat)
				}

				fmt.Println("\tsending notification to the server that we're done transmitting.")

				// But first tell server we're done transmitting. No ACK expected.
				//FIXME: needed to simulate a nonblocking fire-and-forget notification here.
				//Idea from here: https://blog.golang.org/go-concurrency-patterns-timing-out-and
				//go func(cl *Client) {
				c.PeerCmdOut <- &msg.Message{
					Type: msg.TransmitDoneType,
				}
				//}(c)

				break Done
			}
		}
	}

	fmt.Println("EXIT - runningTx")
	return (*Client).done, nil
}

// runningTxRx tracks FSM state where client is both sending and receiving traffic.
func (c *Client) runningTxRx(ctx context.Context) (clientStateFunc, error) {
	c.enter("runningTxRx")

	//FIXME: this is for now. once the OP interface is working select{} will use a OP polling channel
	// to figure out when transmission is done.
	// For now the 3 second timeout is to coordinate with the unit tests.
	testDone := time.After(3 * time.Second)

	for {
		select {
		case notif := <-c.PeerNotifIn:
			switch notif.Type {
			case msg.ErrorType:
				//Write out error.
				return nil, &InternalError{Message: notif.Value.(string)}
			case msg.StopNotificationType:
				//Server is done transmitting. Switch to Tx-only state.
				return (*Client).runningTx, nil
			case msg.StatsNotificationType:
				fmt.Println("got some stats from the server. should be tx and rx server stats.")
				//FIXME add a go routine to handle client side stats?
			}
		case <-testDone:
			//Client is done transmitting. Switch to Rx-only state.
			// But first tell server we're done transmitting. No ACK expected.
			//FIXME: needed to simulate a nonblocking fire-and-forget notification here.
			//Idea from here: https://blog.golang.org/go-concurrency-patterns-timing-out-and
			go func(cl *Client) {
				cl.PeerCmdOut <- &msg.Message{
					Type: msg.TransmitDoneType,
				}
			}(c)
			return (*Client).runningRx, nil
		}
	}

	fmt.Println("EXIT - runningTxRx")
	//Test shouldn't get here. But the compiler gets cranky if there's no return here.
	return nil, &InternalError{Message: "Did not switch out of both sides transmitting state correctly."}
}

// done tracks the FSM state immediately after traffic has stopped. Used to collate results.
func (c *Client) done(ctx context.Context) (clientStateFunc, error) {
	c.enter("done")

	//Get EOT results from server.
	c.PeerCmdOut <- &msg.Message{
		Type: msg.GetFinalStatsType,
	}

	// wait for values
	reply := <-c.PeerRespIn

	if reply.Type != msg.FinalStatsType {
		return nil, &MessagingError{Message: reply.Value.(string)}
	}

	fmt.Println("EXIT - done")

	return (*Client).disconnect, nil
}

// disconnect tracks FSM state that disconnects from the server spiperf instance.
func (c *Client) disconnect(ctx context.Context) (clientStateFunc, error) {
	c.enter("disconnect")

	c.PeerCmdOut <- &msg.Message{
		Type: msg.CleanupType,
	}

	//FIXME: close cmd out channel here?
	fmt.Println("EXIT - disconnect")
	return (*Client).cleanup, nil
}

func (c *Client) cleanup(ctx context.Context) (clientStateFunc, error) {
	c.enter("cleanup")

	//Delete the generator we created.
	reqDone := make(chan struct{})
	req := &openperf.Command{
		Request: &openperf.DeleteGeneratorRequest{
			Id: "Generator-one"},
		Done: reqDone,
		Ctx:  ctx,
	}

	c.OpenperfCmdOut <- req

	timeout := time.After(c.OpenperfTimeout)
	select {
	case <-reqDone:
		if req.Response != nil {
			// error!
		}

	case <-timeout:
		//OP took too long to respond.
	}

	fmt.Println("EXIT - cleanup")
	return nil, nil
}
