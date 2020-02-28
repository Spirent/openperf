package fsm

import (
	"context"
	"errors"
	"fmt"
	"net/url"
	"sync/atomic"
	"time"

	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
	"github.com/go-openapi/strfmt/conv"
)

const TimeFormatString = time.RFC3339Nano

// Client FSM that handles client mode.
type Client struct {
	// PeerCmdOut sends commands to the peer's Server FSM.
	PeerCmdOut chan<- *msg.Message

	// PeerRespIn receives responses from the peer's Server FSM.
	PeerRespIn <-chan *msg.Message

	// PeerNotifIn receives notifications from the peer's Server FSM.
	PeerNotifIn <-chan *msg.Message

	// OpenperfCmdOut sends commands to Openperf.
	OpenperfCmdOut chan<- *openperf.Command

	// DataStreamStatsOut output channel for test data stream statistics.
	DataStreamStatsOut chan<- *Stats

	// PeerTimeout specifies the maximum time the FSM  will wait for responses from the peer's Server FSM.
	PeerTimeout time.Duration

	// OpenperfTimeout specifies the maximum time the FSM will wait for responses from Openperf.
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
		return &InvalidParamError{What: "PeerCmdOut", Actual: "nil", Expected: "non-nil"}
	}
	if c.PeerRespIn == nil {
		return &InvalidParamError{What: "PeerRespIn", Actual: "nil", Expected: "non-nil"}
	}
	if c.PeerNotifIn == nil {
		return &InvalidParamError{What: "PeerNotifIn", Actual: "nil", Expected: "non-nil"}
	}
	if c.OpenperfCmdOut == nil {
		return &InvalidParamError{What: "OpenperfCmdOut", Actual: "nil", Expected: "non-nil"}
	}
	if c.DataStreamStatsOut == nil {
		return &InvalidParamError{What: "DataStreamStatsOut", Actual: "nil", Expected: "non-nil"}
	}
	if c.PeerTimeout < 0 {
		return &InvalidParamError{What: "PeerTimeout", Actual: fmt.Sprintf("%s", c.PeerTimeout), Expected: ">= 0"}
	}
	if c.PeerTimeout == 0 {
		c.PeerTimeout = DefaultPeerTimeout
	}
	if c.OpenperfTimeout < 0 {
		return &InvalidParamError{What: "OpenperfTimeout", Actual: fmt.Sprintf("%s", c.OpenperfTimeout), Expected: ">= 0"}
	}
	if c.OpenperfTimeout == 0 {
		c.OpenperfTimeout = DefaultOpenperfTimeout
	}
	if c.StartDelay < 0 {
		return &InvalidParamError{What: "StartDelay", Actual: fmt.Sprintf("%s", c.StartDelay), Expected: ">= 0"}
	}
	if c.StartDelay == 0 {
		c.StartDelay = DefaultStartDelay
	}
	if c.StatsPollInterval < 0 {
		return &InvalidParamError{What: "StatsPollInterval", Actual: fmt.Sprintf("%s", c.StatsPollInterval), Expected: ">= 0"}
	}
	if c.StatsPollInterval == 0 {
		c.StatsPollInterval = DefaultStatsPollInterval
	}
	if c.GeneratorPollInterval < 0 {
		return &InvalidParamError{What: "GeneratorPollInterval", Actual: fmt.Sprintf("%s", c.GeneratorPollInterval), Expected: ">= 0"}
	}
	if c.GeneratorPollInterval == 0 {
		c.GeneratorPollInterval = DefaultGeneratorPollInterval
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
	reply, err := c.waitForPeerResponse(msg.HelloType)
	if err != nil {
		return nil, err
	}

	//Verify remote version matches our version.
	helloMsg, ok := reply.Value.(*msg.Hello)
	if !ok {
		return nil, &PeerError{What: fmt.Sprintf("message with type %s, ", reply.Type),
			Actual:   fmt.Sprintf("%T", reply.Value),
			Expected: fmt.Sprintf("%T", msg.Hello{})}
	}

	if helloMsg.PeerProtocolVersion != msg.Version {
		return nil, &PeerError{Actual: msg.Version,
			Expected: helloMsg.PeerProtocolVersion,
			What:     "client and server version mismatch"}
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
	reply, repErr := c.waitForPeerResponse(msg.ServerParametersType)
	if repErr != nil {
		return nil, repErr
	}

	serverPrams, msgOk := reply.Value.(*msg.ServerParametersResponse)
	if !msgOk {
		return nil, &PeerError{What: fmt.Sprintf("message with type %s, ", reply.Type),
			Actual:   fmt.Sprintf("%T", reply.Value),
			Expected: fmt.Sprintf("%T", &msg.ServerParametersResponse{})}
	}

	//XXX: full validation of server's parameters will be done here.
	//This line is to exercise the error path via unit test.
	if _, err := url.Parse(serverPrams.OpenperfURL); err != nil {
		return nil, &InvalidConfigurationError{
			What:     "OpenperfURL",
			Actual:   serverPrams.OpenperfURL,
			Expected: "<a valid URL>",
		}
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
	reply, repErr = c.waitForPeerResponse(msg.AckType)
	if repErr != nil {
		return (*Client).cleanup, repErr
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
	case *openperf.GetTimeResponse:
		val := time.Time(conv.DateTimeValue(resp.Time))
		//calculate start time
		c.startTime = val.Add(c.StartDelay)

		//send start time to server
		c.PeerCmdOut <- &msg.Message{
			Type:  msg.StartCommandType,
			Value: &msg.StartCommand{StartTime: c.startTime.Format(TimeFormatString)},
		}
	case error:
		return (*Client).cleanup, &OpenperfError{Err: resp}

	default:
		return (*Client).cleanup, &OpenperfError{
			Actual:   fmt.Sprintf("%T", req.Response),
			Expected: "openperf.GetTimeResponse",
			What:     "sent an openperf.GetTimeRequest and got an unexpected response. "}
	}

	// Wait for an ACK to start message.
	_, ok := c.waitForPeerResponse(msg.AckType)
	if ok != nil {
		return (*Client).cleanup, ok
	}

	// switch to armed state
	return (*Client).armed, nil
}

// armed FSM state that waits for the start time to arrive before switching to running state.
func (c *Client) armed(ctx context.Context) (clientStateFunc, error) {
	c.enter("armed")

	// Wait for then to become now (aka test to start.)
	// Also make sure peer didn't disappear on us.
Done:
	for {
		select {
		case <-time.After(time.Until(c.startTime)):
			break Done

		case notif, ok := <-c.PeerNotifIn:
			if !ok {
				return (*Client).cleanup, &PeerError{What: "error reading peer notifications."}
			}

			switch notif.Type {
			case msg.StatsNotificationType:
				// There is a chance the server will start sending stats
				// before we switch to the running state.
				// This is not an error and we need to handle accordingly.
				if err := c.handleStatsNotification(notif); err != nil {
					return (*Client).cleanup, err
				}

			case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
				// Return an error here. Peer should not have disconnected.
				err := processUnexpectedPeerDisconnect(notif)
				return (*Client).cleanup, err

			default:
				return (*Client).cleanup, &PeerError{
					What:   "unexpected response from peer",
					Actual: notif.Type,
					Expected: fmt.Sprintf("one of: %s, %s, or %s",
						msg.StatsNotificationType,
						msg.PeerDisconnectRemoteType,
						msg.PeerDisconnectLocalType),
				}
			}

		}
	}

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
	case c.TestConfiguration.UpstreamRateBps > 0 && c.TestConfiguration.DownstreamRateBps > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, c.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, c.StatsPollInterval)

		defer txStatsPollCancel()

		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, c.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = true
		serverRunning = true

	// client -> server traffic
	case c.TestConfiguration.UpstreamRateBps > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, c.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, c.StatsPollInterval)

		defer txStatsPollCancel()

		clientRunning = true
		serverRunning = false

	// server -> client traffic
	case c.TestConfiguration.DownstreamRateBps > 0:
		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = c.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, c.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = false
		serverRunning = true
	}

Done:
	for {
		select {
		case notif, ok := <-c.PeerNotifIn:
			if !ok {
				return (*Client).cleanup, &PeerError{Err: errors.New("error reading peer notifications.")}
			}

			switch notif.Type {
			case msg.TransmitDoneType:
				serverRunning = false
				rxStatsPollCancel()

				if !clientRunning {
					break Done
				}

			case msg.StatsNotificationType:
				if err := c.handleStatsNotification(notif); err != nil {
					return (*Client).cleanup, err
				}

			case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
				// Return an error here. Peer should not have disconnected.
				err := processUnexpectedPeerDisconnect(notif)
				return (*Client).cleanup, err

			default:
				return (*Client).cleanup, &PeerError{
					What:   "unexpected response from peer",
					Actual: notif.Type,
					Expected: fmt.Sprintf("one of: %s, %s, %s, or %s",
						msg.TransmitDoneType,
						msg.StatsNotificationType,
						msg.PeerDisconnectRemoteType,
						msg.PeerDisconnectLocalType),
				}
			}
		case txStat, ok := <-txStatsPollResp:
			if !ok {
				txStatsPollResp = nil
				continue
			}

			switch stats := txStat.(type) {
			case *openperf.GetTxStatsResponse:
				c.DataStreamStatsOut <- &Stats{Kind: UpstreamTxKind, Values: stats}
			case error:
				return (*Client).cleanup, &OpenperfError{Err: stats}

			default:
				return (*Client).cleanup, &OpenperfError{
					Actual:   fmt.Sprintf("%T", txStat),
					Expected: "openperf.GetTxStatsResponse",
					What:     "sent an openperf.GetTxStatsRequest and got an unexpected response. ",
				}
			}

		case rxStat, ok := <-rxStatsPollResp:
			if !ok {
				rxStatsPollResp = nil
				continue
			}

			switch stats := rxStat.(type) {
			case *openperf.GetRxStatsResponse:
				c.DataStreamStatsOut <- &Stats{Kind: DownstreamRxKind, Values: stats}
			case error:
				return (*Client).cleanup, &OpenperfError{Err: stats}

			default:
				return (*Client).cleanup, &OpenperfError{
					Actual:   fmt.Sprintf("%T", rxStat),
					Expected: "openperf.GetRxStatsResponse",
					What:     "sent an openperf.GetRxStatsRequest and got an unexpected response. ",
				}

			}

		case gen, ok := <-generatorPollResp:
			if !ok {
				generatorPollResp = nil
				continue
			}

			switch generator := gen.(type) {
			case *openperf.GetGeneratorResponse:

				if generator.Running {
					continue
				}

				if !clientRunning {
					// Handle case where an extra poll response comes through
					// even after canceling. This is not an error.
					continue
				}

				//Stop local tx stats polling.
				txStatsPollCancel()

				//Stop runstate polling.
				generatorPollCancel()

				// Check if there's an in-flight GetTxStatsResponse waiting for us.
				// Not having it is not an error.
				lastStat, ok := <-txStatsPollResp
				if ok {
					switch stat := lastStat.(type) {
					case *openperf.GetTxStatsResponse:
						c.DataStreamStatsOut <- &Stats{Kind: UpstreamTxKind, Values: stat}
					case error:
						return (*Client).cleanup, &OpenperfError{Err: stat}

					default:
						return (*Client).cleanup, &OpenperfError{
							Actual:   fmt.Sprintf("%T", lastStat),
							Expected: "openperf.GetTxStatsResponse",
							What:     "sent an openperf.GetTxStatsRequest and got an unexpected response. ",
						}

					}
				}

				// Tell server we're done transmitting. No ACK expected.
				c.PeerCmdOut <- &msg.Message{
					Type: msg.TransmitDoneType,
				}

				clientRunning = false
				if !serverRunning {
					break Done
				}

			case error:
				return (*Client).cleanup, &OpenperfError{Err: generator}

			default:
				return (*Client).cleanup, &OpenperfError{
					Actual:   fmt.Sprintf("%T", gen),
					Expected: "openperf.GetGeneratorResponse",
					What:     "sent an openperf.GetGeneratorRequest and got an unexpected response. ",
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
	if err := c.getFinalServerStats(); err != nil {
		return (*Client).cleanup, err
	}

	//Get EOT results from client (aka local Openperf).
	if err := c.getFinalClientTxStats(ctx); err != nil {
		return (*Client).cleanup, err
	}

	if err := c.getFinalClientRxStats(ctx); err != nil {
		return (*Client).cleanup, err
	}

	return (*Client).cleanup, nil
}

func (c *Client) cleanup(ctx context.Context) (clientStateFunc, error) {
	c.enter("cleanup")

	if c.TestConfiguration.UpstreamRateBps > 0 {
		//Delete the generator we created.
		reqDone := make(chan struct{})
		req := &openperf.Command{
			Request: &openperf.DeleteGeneratorRequest{
				Id: "Generator-one"}, //XXX: hardcoded for now as a placeholder.
			Done: reqDone,
			Ctx:  ctx,
		}

		c.OpenperfCmdOut <- req

		// Wait for request to finish one way or another. OP controller ensures this never gets stuck.
		<-reqDone

		if req.Response != nil {
			return nil, &OpenperfError{Err: req.Response.(error)}
		}

	}

	return nil, nil
}

func (c *Client) waitForPeerResponse(expectedType string) (*msg.Message, error) {

	for {
		select {
		case resp, ok := <-c.PeerRespIn:
			if !ok {
				return nil, &PeerError{What: "error reading from peer response channel."}
			}

			switch resp.Type {
			case expectedType:
				return resp, nil

			case msg.ErrorType:
				val, ok := resp.Value.(error)
				if !ok {
					return nil, &PeerError{}
				}
				return nil, &PeerError{Err: val}
			}

			return nil, &PeerError{
				What:     "unexpected response from peer",
				Actual:   resp.Type,
				Expected: expectedType}

		case notif, ok := <-c.PeerNotifIn:

			if !ok {
				return nil, &PeerError{What: "error reading from peer notification channel."}
			}

			switch notif.Type {
			case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
				return nil, processUnexpectedPeerDisconnect(notif)

			case msg.StatsNotificationType:
				// Sometimes upstream-only tests can have an in-flight stats notification from
				// the server when the TransmitDoneType message is sent.
				// Handle it here lest the stats are lost.
				if err := c.handleStatsNotification(notif); err != nil {
					return nil, err
				}
			default:
				return nil, &PeerError{
					What:   "unexpected response from peer",
					Actual: notif.Type,
					Expected: fmt.Sprintf("one of: %s, %s, or %s",
						msg.PeerDisconnectRemoteType,
						msg.PeerDisconnectLocalType,
						msg.StatsNotificationType),
				}

			}

		case <-time.After(c.PeerTimeout):
			return nil, &TimeoutError{Operation: "waiting for a reply from peer."}
		}
	}

	return nil, &InternalError{Message: "waiting for a peer response."}
}

func (c *Client) handleStatsNotification(notif *msg.Message) error {
	stats, ok := notif.Value.(*msg.DataStreamStats)
	if !ok {
		return &PeerError{
			What:     fmt.Sprintf("message with type %s, ", notif.Type),
			Actual:   fmt.Sprintf("%T", notif.Value),
			Expected: fmt.Sprintf("%T", &msg.DataStreamStats{}),
		}
	}

	if stats.TxStats != nil {
		c.DataStreamStatsOut <- &Stats{Kind: DownstreamTxKind, Values: stats.TxStats}
	}

	if stats.RxStats != nil {
		c.DataStreamStatsOut <- &Stats{Kind: UpstreamRxKind, Values: stats.RxStats}
	}

	return nil
}

func processUnexpectedPeerDisconnect(notif *msg.Message) error {

	switch notif.Type {
	case msg.PeerDisconnectLocalType:
		peerDisconn, ok := notif.Value.(*msg.PeerDisconnectLocalNotif)
		if !ok {
			return &PeerError{
				What:     fmt.Sprintf("message with type %s, ", notif.Type),
				Actual:   fmt.Sprintf("%T", notif.Value),
				Expected: fmt.Sprintf("%T", &msg.PeerDisconnectLocalNotif{}),
			}
		}

		return &UnexpectedPeerDisconnectError{Err: peerDisconn.Err, Local: true}

	case msg.PeerDisconnectRemoteType:
		peerDisconn, ok := notif.Value.(*msg.PeerDisconnectRemoteNotif)
		if !ok {
			return &PeerError{
				What:     fmt.Sprintf("message with type %s, ", notif.Type),
				Actual:   fmt.Sprintf("%T", notif.Value),
				Expected: fmt.Sprintf("%T", &msg.PeerDisconnectRemoteNotif{}),
			}
		}

		return &UnexpectedPeerDisconnectError{Err: errors.New(peerDisconn.Err), Local: false}

	default:
		return &PeerError{
			What:   "unexpected response from peer",
			Actual: notif.Type,
			Expected: fmt.Sprintf("one of: %s or %s",
				msg.PeerDisconnectRemoteType,
				msg.PeerDisconnectLocalType),
		}
	}
}

func (c *Client) getFinalServerStats() error {
	c.PeerCmdOut <- &msg.Message{
		Type: msg.GetFinalStatsType,
	}

	//Wait for EOT results from server.
	resp, respErr := c.waitForPeerResponse(msg.FinalStatsType)
	if respErr != nil {
		return respErr
	}

	stats, ok := resp.Value.(*msg.FinalStats)
	if !ok {
		return &PeerError{
			What:     fmt.Sprintf("message with type %s, ", resp.Type),
			Actual:   fmt.Sprintf("%T", resp.Value),
			Expected: fmt.Sprintf("%T", &msg.FinalStats{}),
		}
	}

	if stats.TxStats != nil {
		c.DataStreamStatsOut <- &Stats{Kind: DownstreamTxKind, Values: stats.TxStats, Final: true}
	}

	if stats.RxStats != nil {
		c.DataStreamStatsOut <- &Stats{Kind: UpstreamRxKind, Values: stats.RxStats, Final: true}
	}

	return nil
}

func (c *Client) getFinalClientTxStats(ctx context.Context) error {

	// Do we need to get TxStats?
	if c.TestConfiguration.UpstreamRateBps <= 0 {
		return nil
	}

	reqDone := make(chan struct{})
	opCtx, opCancel := context.WithTimeout(ctx, c.OpenperfTimeout)
	defer opCancel()

	req := &openperf.Command{
		Request: &openperf.GetTxStatsRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	c.OpenperfCmdOut <- req

	<-reqDone

	switch resp := req.Response.(type) {
	case *openperf.GetTxStatsResponse:
		c.DataStreamStatsOut <- &Stats{Kind: UpstreamTxKind, Values: resp, Final: true}

	case error:
		return resp

	default:
		return &OpenperfError{
			Actual:   fmt.Sprintf("%T", req.Response),
			Expected: "openperf.GetTxStatsResponse",
			What:     "sent an openperf.GetTxStatsRequest and got an unexpected response. ",
		}
	}

	return nil
}

func (c *Client) getFinalClientRxStats(ctx context.Context) error {

	// Do we need to get RxStats?
	if c.TestConfiguration.DownstreamRateBps <= 0 {
		return nil
	}

	reqDone := make(chan struct{})
	opCtx, opCancel := context.WithTimeout(ctx, c.OpenperfTimeout)
	defer opCancel()

	req := &openperf.Command{
		Request: &openperf.GetRxStatsRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	c.OpenperfCmdOut <- req

	<-reqDone

	switch resp := req.Response.(type) {
	case *openperf.GetRxStatsResponse:
		c.DataStreamStatsOut <- &Stats{Kind: DownstreamRxKind, Values: resp, Final: true}

	case error:
		return resp

	default:
		return &OpenperfError{
			Actual:   fmt.Sprintf("%T", req.Response),
			Expected: "openperf.GetRxStatsResponse",
			What:     "sent an openperf.GetRxStatsRequest and got an unexpected response. ",
		}
	}

	return nil
}

func (c *Client) startOpenperfCmdRepeater(ctx context.Context, cmd *openperf.Command, interval time.Duration) (responses chan interface{}, cancelFn context.CancelFunc) {
	var requestCtx context.Context
	requestCtx, cancelFn = context.WithCancel(ctx)
	responses = make(chan interface{})
	repeater := &openperf.CommandRepeater{
		Command:        cmd,
		Interval:       interval,
		OpenperfCmdOut: c.OpenperfCmdOut,
		Responses:      responses}

	go repeater.Run(requestCtx)

	return

}
