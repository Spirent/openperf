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

const Version = "1.0"

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

	// StatsPollInterval how often to poll relevant local statistics. This does not impact results output.
	StatsPollInterval time.Duration

	// GeneratorPollInterval how often to poll the generator resource to see if it's still transmitting.
	GeneratorPollInterval time.Duration

	// state tracks the state machine's current state as a string.
	state atomic.Value

	// startTime is the time at which traffic will begin transmitting.
	startTime time.Time
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
	if c.GeneratorPollInterval <= 0 {
		return &InternalError{Message: "Invalid generator polling interval"}
	}
	if c.StatsPollInterval <= 0 {
		return &InternalError{Message: "Invalid stats polling interval"}
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
			Version: Version,
		},
	}

	// Wait for reply
	timeout := time.After(c.PeerTimeout)
	select {
	case reply, ok := <-c.PeerRespIn:
		if !ok {
			return nil, &InternalError{Message: "Error reading from peer response channel"}
		}

		if reply.Type == msg.ErrorType {
			return nil, &InternalError{Message: reply.Value.(string)}
		}

		// Verify remote version matches our version.
		if serverVersion := reply.Value.(*msg.Hello).Version; serverVersion != Version {
			return nil, &InternalError{Message: fmt.Sprintf("Mismatch between server and client versions. Client version is %s and server reports as %s\n", Version, serverVersion)}
		}

	case <-timeout:
		return nil, &TimeoutError{Message: "Timed out waiting for server reply to Hello message."}
	}

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
		//XXX: full validation of server's parameters will be done here.
		//This line is to exercise the error path via unit test.
		if _, err := url.Parse(serverPrams.OpenperfURL); err != nil {
			return nil, &InvalidConfigurationError{Message: fmt.Sprintf("Got invalid OpenPerf URL from server: %s", serverPrams.OpenperfURL)}
		}

	case <-timeout:
		return nil, &TimeoutError{Message: "Timed out waiting for server reply to get parameters message."}
	}

	//XXX: test configuration will be built here.

	// Send configuration to server.
	c.PeerCmdOut <- &msg.Message{
		Type:  msg.SetConfigType,
		Value: &msg.ServerConfiguration{},
	}

	// Wait for an ACK.
	reply := <-c.PeerRespIn

	if reply.Type != msg.AckType {
		return nil, &InvalidConfigurationError{Message: reply.Value.(string)}
	}

	// Switch to Ready state.
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
		//calculate start time
		c.startTime = val.Add(c.StartDelay)

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

	// switch to armed state
	return (*Client).armed, nil
}

// armed FSM state that waits for the start time to arrive before switching to running state.
func (c *Client) armed(ctx context.Context) (clientStateFunc, error) {
	c.enter("armed")

	// Wait for then to become now (aka test to start.)
	time.Sleep(time.Until(c.startTime))

	return (*Client).running, nil
}

// running tracks FSM state where client is only sending traffic.
func (c *Client) running(ctx context.Context) (clientStateFunc, error) {
	c.enter("running")

	// generatorPollResp channel to receive responses from polling Openperf generator resource.
	var generatorPollResp chan interface{}
	// generatorPollCancel stop generator polling.
	var generatorPollCancel context.CancelFunc

	// txStatsPollResp channel to receive responses from polling local Openperf transmit stats.
	var txStatsPollResp chan interface{}
	// txStatsPollCancel stop polling local Openperf transmit stats.
	var txStatsPollCancel context.CancelFunc

	// rxStatsPollResp channel to receive responses from polling local Openperf receive stats.
	var rxStatsPollResp chan interface{}
	// rxStatsPollCancel stop polling local Openperf receive stats.
	var rxStatsPollCancel context.CancelFunc

	var clientRunning bool
	var serverRunning bool

	switch {
	case c.TestConfiguration.UpstreamRate > 0 && c.TestConfiguration.DownstreamRate > 0:
		// Start up generator polling (to check runstate)
		var generatorReqCtx context.Context
		generatorReqCtx, generatorPollCancel = context.WithCancel(ctx)
		generatorPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(generatorReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetGeneratorRequest{}},
			Interval:           c.GeneratorPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          generatorPollResp})

		// Start up transmit stats polling
		var txStatReqCtx context.Context
		txStatReqCtx, txStatsPollCancel = context.WithCancel(ctx)
		txStatsPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(txStatReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetTxStatsRequest{}},
			Interval:           c.StatsPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          txStatsPollResp})

		// Start up receive stats polling
		var rxStatReqCtx context.Context
		rxStatReqCtx, rxStatsPollCancel = context.WithCancel(ctx)
		rxStatsPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(rxStatReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetRxStatsRequest{}},
			Interval:           c.StatsPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          rxStatsPollResp})

		clientRunning = true
		serverRunning = true

	case c.TestConfiguration.UpstreamRate > 0:
		var generatorReqCtx context.Context
		generatorReqCtx, generatorPollCancel = context.WithCancel(ctx)
		generatorPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(generatorReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetGeneratorRequest{}},
			Interval:           c.GeneratorPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          generatorPollResp})

		var txStatReqCtx context.Context
		txStatReqCtx, txStatsPollCancel = context.WithCancel(ctx)
		txStatsPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(txStatReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetTxStatsRequest{}},
			Interval:           c.StatsPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          txStatsPollResp})

		clientRunning = true
		serverRunning = false

	case c.TestConfiguration.DownstreamRate > 0:
		var rxStatReqCtx context.Context
		rxStatReqCtx, rxStatsPollCancel = context.WithCancel(ctx)
		rxStatsPollResp = make(chan interface{})
		go openperf.RunCommandRepeater(rxStatReqCtx, &openperf.CommandRepeater{
			Command:            &openperf.Command{Request: &openperf.GetRxStatsRequest{}},
			Interval:           c.StatsPollInterval,
			OpenperfController: c.OpenperfCmdOut,
			Responses:          rxStatsPollResp})

		clientRunning = false
		serverRunning = true
	}

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
			case msg.TransmitDoneType:
				serverRunning = false
				rxStatsPollCancel()
				//FIXME: check for redundant messages here?

				//fmt.Println("got msg.TransmitDoneType")
				if !clientRunning {
					break Done
				}
			case msg.StatsNotificationType:
				//FIXME add a go routine to handle client side stats?
			}
		case txStat, ok := <-txStatsPollResp:
			if !ok {
				txStatsPollResp = nil
				continue
			}
			pretty.Println(txStat)
			//FIXME do something with the local tx stats.
		case rxStat, ok := <-rxStatsPollResp:
			if !ok {
				rxStatsPollResp = nil
				continue
			}
			pretty.Println(rxStat)
			//FIXME do something with the local rx stats.
		case gen, ok := <-generatorPollResp:
			if !ok {
				generatorPollResp = nil //FIXME: this might be redundant.
				continue
			}

			generator, genOk := gen.(*openperf.GetGeneratorResponse)
			if !genOk {
				generatorPollCancel()
				txStatsPollCancel()
				return nil, &InternalError{Message: gen.(error).Error()}
			}

			if !generator.Running {
				if !clientRunning {
					// Handle case where an extra poll response comes through
					// even after canceling. This is not an error.
					continue
				}

				//Stop local tx stats polling.
				txStatsPollCancel()

				//Stop runstate polling.
				generatorPollCancel()

				lastStat, ok := <-txStatsPollResp
				if ok {
					//do something with lastStat
					fmt.Print("last stat: ")
					pretty.Println(lastStat)
				}

				// Tell server we're done transmitting. No ACK expected.
				c.PeerCmdOut <- &msg.Message{
					Type: msg.TransmitDoneType,
				}

				clientRunning = false
				if !serverRunning {
					break Done
				}
			}
		}
	}

	return (*Client).done, nil
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

	return (*Client).disconnect, nil
}

// disconnect tracks FSM state that disconnects from the server spiperf instance.
func (c *Client) disconnect(ctx context.Context) (clientStateFunc, error) {
	c.enter("disconnect")

	c.PeerCmdOut <- &msg.Message{
		Type: msg.CleanupType,
	}

	//FIXME: close cmd out channel here?
	return (*Client).cleanup, nil
}

func (c *Client) cleanup(ctx context.Context) (clientStateFunc, error) {
	c.enter("cleanup")

	if c.TestConfiguration.UpstreamRate > 0 {
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
	}

	return nil, nil
}
