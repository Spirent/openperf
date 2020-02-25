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

// Client is the FSM for spiperf when run in client mode.
type Client struct {
	// PeerCmdOut sends commands to the peer's server FSM.
	PeerCmdOut chan<- *msg.Message

	// PeerRespIn receives responses from the server Spiperf instance.
	PeerRespIn <-chan *msg.Message

	// PeerNotifIn receives notifications from the server Spiperf instance.
	PeerNotifIn <-chan *msg.Message

	// OpenperfCmdOut sends commands to Openperf Controller.
	OpenperfCmdOut chan<- *openperf.Command

	// PeerTimeout specifies the maximum time the FSM  will wait for responses from the peer instance.
	PeerTimeout time.Duration

	// OpenperfTimeout specifies the maximum time the FSM will wait for responses from the Openperf instance.
	OpenperfTimeout time.Duration

	// StartDelay added to start time so server and client start traffic generation and analysis at the same time.
	StartDelay time.Duration

	// TestConfiguration keeps track of the test configuration.
	TestConfiguration Configuration

	// StatsPollInterval how often to poll local Openperf statistics. This does not impact results output.
	StatsPollInterval time.Duration

	// GeneratorPollInterval how often to poll local Openperf generator resource to see if it's still transmitting.
	GeneratorPollInterval time.Duration

	// state tracks the state machine's current state as a string.
	state atomic.Value

	// startTime is the time at which traffic will begin transmitting.
	startTime time.Time
}

func (c *Client) enter(state string) {
	c.state.Store(state)
}

func (c *Client) State() (state string) {
	state, _ = c.state.Load().(string)
	return
}

type clientStateFunc func(*Client, context.Context) (clientStateFunc, error)

func (c *Client) Run(ctx context.Context) error {

	if c.PeerCmdOut == nil {
		return &InternalError{Message: "invalid command out channel"}
	}
	if c.PeerRespIn == nil {
		return &InternalError{Message: "invalid response in channel"}
	}
	if c.PeerNotifIn == nil {
		return &InternalError{Message: "invalid notification in channel"}
	}
	if c.PeerTimeout <= 0 {
		return &InternalError{Message: "invalid peer timeout"}
	}
	if c.PeerTimeout == 0 {
		c.PeerTimeout = DefaultPeerTimeout
	}
	if c.OpenperfTimeout <= 0 {
		return &InternalError{Message: "invalid openperf timeout"}
	}
	if c.OpenperfTimeout == 0 {
		c.OpenperfTimeout = DefaultOpenperfTimeout
	}
	if c.GeneratorPollInterval <= 0 {
		return &InternalError{Message: "invalid generator polling interval"}
	}
	if c.GeneratorPollInterval == 0 {
		c.GeneratorPollInterval = DefaultGeneratorPollInterval
	}
	if c.StatsPollInterval <= 0 {
		return &InternalError{Message: "invalid stats polling interval"}
	}
	if c.StatsPollInterval == 0 {
		c.StatsPollInterval = DefaultStatsPollInterval
	}

	defer close(c.PeerCmdOut)

	var retVal error
	f := (*Client).connect
	for f != nil {
		var err error
		//Goal here is to keep the first error since any subsequent errors are probably a result of that.
		if f, err = f(c, ctx); err != nil && retVal == nil {
			retVal = err
		}
	}

	return retVal
}

func (c *Client) connect(ctx context.Context) (clientStateFunc, error) {
	c.enter("connect")

	// Exchange version information with our peer to verify compatibility.
	c.PeerCmdOut <- &msg.Message{
		Type: msg.HelloType,
		Value: &msg.Hello{
			PeerProtocolVersion: msg.Version,
		},
	}

	// Wait for reply
	reply, ok := c.waitForPeerResponse()
	if ok != nil {
		return nil, ok
	}

	switch reply.Type {
	case msg.HelloType:
		//Verify remote version matches our version.
		if helloMsg, ok := reply.Value.(*msg.Hello); ok {
			if helloMsg.PeerProtocolVersion != msg.Version {
				return nil, &VersionMismatchError{Message: fmt.Sprintf("Mismatch between server and client versions. Client version is %s and server reports as %s\n", msg.Version, helloMsg.PeerProtocolVersion)}
			}
		} else {
			return nil, &MessagingError{Message: "Unexpected value type in hello message reply"}
		}

	case msg.ErrorType:
		return nil, &PeerError{Message: reply.Value.(string)}
	default:
		return nil, &UnexpectedPeerRespError{Message: fmt.Sprintf("got unexpected message type %s. Expected %s.", reply.Type, msg.HelloType)}
	}

	return (*Client).configure, nil
}

func (c *Client) configure(ctx context.Context) (clientStateFunc, error) {
	c.enter("configure")

	// Get server's configuration parameters (i.e. CLI arguments)
	c.PeerCmdOut <- &msg.Message{
		Type: msg.GetConfigType,
	}

	// Wait for response.
	reply, ok := c.waitForPeerResponse()
	if ok != nil {
		return nil, ok
	}

	switch reply.Type {
	case msg.ServerParametersResponseType:
		if serverPrams, ok := reply.Value.(*msg.ServerParametersResponse); ok {
			//XXX: full validation of server's parameters will be done here.
			//This line is to exercise the error path via unit test.
			if _, err := url.Parse(serverPrams.OpenperfURL); err != nil {
				return nil, &InvalidConfigurationError{Message: fmt.Sprintf("Got invalid OpenPerf URL from server: %s", serverPrams.OpenperfURL)}
			}
		} else {
			return nil, &MessagingError{Message: "Unexpected value type in server parameters reply"}
		}

	case msg.ErrorType:
		return nil, &PeerError{Message: reply.Value.(string)}

	default:
		return nil, &UnexpectedPeerRespError{Message: fmt.Sprintf("got unexpected message type %s. Expected %s.", reply.Type, msg.ServerParametersResponseType)}
	}

	//XXX: test configuration will be built here.

	//XXX: local Openperf will be configured here.
	//From here on out any error must jump to the cleanup state before exiting.

	// Send configuration to server.
	c.PeerCmdOut <- &msg.Message{
		Type:  msg.SetConfigType,
		Value: &msg.ServerConfiguration{},
	}

	// Wait for an ACK.
	reply, ok = c.waitForPeerResponse()
	if ok != nil {
		return (*Client).cleanup, ok
	}

	switch reply.Type {

	case msg.AckType:

	case msg.ErrorType:
		return (*Client).cleanup, &PeerError{Message: reply.Value.(string)}

	default:
		return (*Client).cleanup, &UnexpectedPeerRespError{Message: fmt.Sprintf("got unexpected message type %s. Expected %s.", reply.Type, msg.AckType)}
	}

	// Switch to Ready state.
	return (*Client).ready, nil
}

func (c *Client) ready(ctx context.Context) (clientStateFunc, error) {
	c.enter("ready")

	reqDone := make(chan struct{})
	opCtx, opCancel := context.WithTimeout(ctx, c.OpenperfTimeout)
	defer opCancel()
	req := &openperf.Command{
		Request: &openperf.GetTimeRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	c.OpenperfCmdOut <- req

	// Wait for request to finish one way or another. OP controller ensures this never gets stuck.
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
		return (*Client).cleanup, &InternalError{Message: resp.Error()}
	}

	// Wait for an ACK to start message.
	reply, ok := c.waitForPeerResponse()
	if ok != nil {
		return (*Client).cleanup, ok
	}

	switch reply.Type {

	case msg.AckType:

	case msg.ErrorType:
		return (*Client).cleanup, &PeerError{Message: reply.Value.(string)}

	default:
		return (*Client).cleanup, &UnexpectedPeerRespError{Message: fmt.Sprintf("got unexpected message type %s. Expected %s.", reply.Type, msg.AckType)}
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

// running tracks FSM state where data stream(s) are transmitting (aka test is running.)
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

	var (
		clientRunning bool
		serverRunning bool
	)

	switch {
	// client <--> server traffic
	case c.TestConfiguration.UpstreamRate > 0 && c.TestConfiguration.DownstreamRate > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, c.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, c.StatsPollInterval)

		defer txStatsPollCancel()

		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, c.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = true
		serverRunning = true

	// client -> server traffic
	case c.TestConfiguration.UpstreamRate > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, c.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, c.StatsPollInterval)

		defer txStatsPollCancel()

		clientRunning = true
		serverRunning = false

	// server -> client traffic
	case c.TestConfiguration.DownstreamRate > 0:
		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = c.makeOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, c.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = false
		serverRunning = true
	}

Done:
	for {
		select {
		case notif, ok := <-c.PeerNotifIn:
			if !ok {
				return (*Client).cleanup, &InternalError{Message: "error reading peer spiperf notifications."}
			}

			switch notif.Type {
			case msg.ErrorType:
				return (*Client).cleanup, &InternalError{Message: notif.Value.(string)}
			case msg.TransmitDoneType:
				serverRunning = false
				rxStatsPollCancel()

				if !clientRunning {
					break Done
				}
			case msg.StatsNotificationType:
				//XXX: statistics from the server will be processed here.
			}
		case txStat, ok := <-txStatsPollResp:
			if !ok {
				txStatsPollResp = nil
				continue
			}

			stats, ok := txStat.(*openperf.GetTxStatsResponse)
			if !ok {
				return (*Client).cleanup, &InternalError{Message: txStat.(error).Error()}
			}
			pretty.Println(stats)
			//XXX: Tx stats will be processed here.

		case rxStat, ok := <-rxStatsPollResp:
			if !ok {
				rxStatsPollResp = nil
				continue
			}
			stats, ok := rxStat.(*openperf.GetRxStatsResponse)
			if !ok {
				return (*Client).cleanup, &InternalError{Message: rxStat.(error).Error()}
			}
			pretty.Println(stats)
			//XXX: Rx stats will be processed here.

		case gen, ok := <-generatorPollResp:
			if !ok {
				generatorPollResp = nil
				continue
			}

			generator, genOk := gen.(*openperf.GetGeneratorResponse)
			if !genOk {
				return (*Client).cleanup, &InternalError{Message: gen.(error).Error()}
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

	//Wait for EOT results from server.
	reply, ok := c.waitForPeerResponse()
	if ok != nil {
		return (*Client).cleanup, ok
	}

	switch reply.Type {
	case msg.FinalStatsType:
		//XXX: final stats will be processed here.
	case msg.ErrorType:
		return (*Client).cleanup, &PeerError{Message: reply.Value.(string)}

	default:
		return (*Client).cleanup, &UnexpectedPeerRespError{Message: fmt.Sprintf("got unexpected message type %s. Expected %s.", reply.Type, msg.FinalStatsType)}
	}

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
				return nil, &InternalError{Message: req.Response.(error).Error()}
			}

		case <-timeout:
			//OP took too long to respond.
		}
	}

	return nil, nil
}

func (c *Client) waitForPeerResponse() (*msg.Message, error) {

	select {
	case resp, ok := <-c.PeerRespIn:
		if !ok {
			return nil, &MessagingError{Message: "error reading from peer response channel."}
		}

		return resp, nil
	case <-time.After(c.PeerTimeout):
		return nil, &TimeoutError{Message: "waiting for a reply from peer."}
	}
}

func (c *Client) makeOpenperfCmdRepeater(ctx context.Context, cmd *openperf.Command, interval time.Duration) (responses chan interface{}, cancelFn context.CancelFunc) {
	var requestCtx context.Context
	requestCtx, cancelFn = context.WithCancel(ctx)
	responses = make(chan interface{})
	go openperf.RunCommandRepeater(requestCtx, &openperf.CommandRepeater{
		Command:            cmd,
		Interval:           interval,
		OpenperfController: c.OpenperfCmdOut,
		Responses:          responses})

	return

}
