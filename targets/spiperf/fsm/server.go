package fsm

import (
	"context"
	"errors"
	"fmt"
	"sync/atomic"
	"time"

	"github.com/Spirent/openperf/targets/spiperf/msg"
	"github.com/Spirent/openperf/targets/spiperf/openperf"
	"github.com/rs/zerolog"
)

// Server FSM that handles server mode.
type Server struct {
	// PeerCmdIn receives commands from the peer's Client FSM.
	PeerCmdIn <-chan *msg.Message

	// PeerRespOut sends responses to the peer's Client FSM.
	PeerRespOut chan<- *msg.Message

	// PeerNotifOut sends notifications to the peer's Client FSM.
	PeerNotifOut chan<- *msg.Message

	// OpenperfCmdOut sends commands to Openperf.
	OpenperfCmdOut chan<- *openperf.Command

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

	// Logger log output facility
	Logger *zerolog.Logger

	// state tracks the state machine's current state as a string.
	state atomic.Value

	// startTime is the time at which traffic will begin transmitting.
	startTime time.Time

	// errorAfterCleanup should the FSM terminate after the cleanup state?
	errorAfterCleanup bool
}

func (s *Server) enter(state string) {
	s.Logger.Trace().Msg(state)
	s.state.Store(state)
}

func (s *Server) State() (state string) {
	state, _ = s.state.Load().(string)
	return
}

type serverStateFunc func(*Server, context.Context) (serverStateFunc, error)

func (s *Server) Run(ctx context.Context) error {

	if s.Logger == nil {
		return &InvalidParamError{What: "Logger", Actual: "nil", Expected: "non-nil"}
	}
	if s.PeerCmdIn == nil {
		return &InvalidParamError{What: "PeerCmdIn", Actual: "nil", Expected: "non-nil"}
	}
	if s.PeerRespOut == nil {
		return &InvalidParamError{What: "PeerRespOut", Actual: "nil", Expected: "non-nil"}
	}
	if s.PeerNotifOut == nil {
		return &InvalidParamError{What: "PeerNotifOut", Actual: "nil", Expected: "non-nil"}
	}
	if s.OpenperfCmdOut == nil {
		return &InvalidParamError{What: "OpenperfCmdOut", Actual: "nil", Expected: "non-nil"}
	}
	if s.PeerTimeout < 0 {
		return &InvalidParamError{What: "PeerTimeout", Actual: fmt.Sprintf("%s", s.PeerTimeout), Expected: ">= 0"}
	}
	if s.PeerTimeout == 0 {
		s.PeerTimeout = DefaultPeerTimeout
		s.Logger.Debug().
			Dur("PeerTimeout", DefaultPeerTimeout).
			Msg("using default value for server FSM parameter")
	}
	if s.OpenperfTimeout < 0 {
		return &InvalidParamError{What: "OpenperfTimeout", Actual: fmt.Sprintf("%s", s.OpenperfTimeout), Expected: ">= 0"}
	}
	if s.OpenperfTimeout == 0 {
		s.OpenperfTimeout = DefaultOpenperfTimeout
		s.Logger.Debug().
			Dur("OpenperfTimeout", DefaultOpenperfTimeout).
			Msg("using default value for server FSM parameter")
	}
	if s.StartDelay < 0 {
		return &InvalidParamError{What: "StartDelay", Actual: fmt.Sprintf("%s", s.StartDelay), Expected: ">= 0"}
	}
	if s.StartDelay == 0 {
		s.StartDelay = DefaultStartDelay
		s.Logger.Debug().
			Dur("StartDelay", DefaultStartDelay).
			Msg("using default value for server FSM parameter")
	}
	if s.StatsPollInterval < 0 {
		return &InvalidParamError{What: "StatsPollInterval", Actual: fmt.Sprintf("%s", s.StatsPollInterval), Expected: ">= 0"}
	}
	if s.StatsPollInterval == 0 {
		s.StatsPollInterval = DefaultStatsPollInterval
		s.Logger.Debug().
			Dur("StatsPollInterval", DefaultStatsPollInterval).
			Msg("using default value for server FSM parameter")
	}
	if s.GeneratorPollInterval < 0 {
		return &InvalidParamError{What: "GeneratorPollInterval", Actual: fmt.Sprintf("%s", s.GeneratorPollInterval), Expected: ">= 0"}
	}
	if s.GeneratorPollInterval == 0 {
		s.GeneratorPollInterval = DefaultGeneratorPollInterval
		s.Logger.Debug().
			Dur("GeneratorPollInterval", DefaultGeneratorPollInterval).
			Msg("using default value for server FSM parameter")
	}

	defer close(s.PeerRespOut)

	var retVal error
	f := (*Server).connect
	for f != nil {
		var err error
		//Goal here is to keep the first error since any subsequent errors are probably a result of that.
		//Note this is only for fatal errors. Non-fatal errors are logged and the FSM resets.
		if f, err = f(s, ctx); err != nil && retVal == nil {
			s.errorAfterCleanup = true
			retVal = err
		}
	}

	return retVal
}

func (s *Server) connect(ctx context.Context) (serverStateFunc, error) {
	s.enter("connect")

	// Wait for client to connect and send a Hello message.
	// Doing this inline since we have no idea when a client will connect,
	// and thus can't set a timeout here.
	select {
	case cmd, ok := <-s.PeerCmdIn:
		if !ok {
			s.Logger.Error().Msg("error reading from peer command channel.")
			return nil, &PeerError{What: "error reading from peer command channel."}
		}

		helloMsg, ok := cmd.Value.(*msg.Hello)
		if !ok {
			s.Logger.Warn().
				Str("actual type", cmd.Type).
				Str("expected type", "HelloType").
				Msg("got unexpected message from peer. Connection terminated.")

			s.PeerRespOut <- &msg.Message{
				Type:  msg.ErrorType,
				Value: fmt.Sprintf("unexpected message type. got %s, expected %s", cmd.Type, msg.HelloType),
			}

			return (*Server).connect, nil
		}

		if helloMsg.PeerProtocolVersion != msg.Version {

			s.Logger.Warn().
				Str("local version", msg.Version).
				Str("remote version", helloMsg.PeerProtocolVersion).
				Msg("peer message version mismatch. Connection terminated.")

			s.PeerRespOut <- &msg.Message{
				Type:  msg.ErrorType,
				Value: fmt.Sprintf("peer version mismatch. got %s, expected %s", helloMsg.PeerProtocolVersion, msg.Version),
			}

			return (*Server).connect, nil
		}

	case <-ctx.Done():
		// We've been canceled. This is not an error.
		s.Logger.Info().
			Msg("Server FSM exiting due to caller cancellation")

		return nil, nil
	}

	s.PeerRespOut <- &msg.Message{
		Type: msg.HelloType,
		Value: &msg.Hello{
			PeerProtocolVersion: msg.Version,
		},
	}

	return (*Server).configure, nil
}

func (s *Server) configure(ctx context.Context) (serverStateFunc, error) {
	s.enter("configure")

	cmd, cmdErr := s.waitForPeerCommand(msg.GetServerParametersType)
	if cmdErr != nil {
		// was the error fatal?
		if cmd == nil {
			s.Logger.Error().
				Err(cmdErr).
				Msg("fatal server FSM error. terminating.")

			return nil, cmdErr
		}

		s.Logger.Error().
			Err(cmdErr).
			Str("expected command type", msg.GetServerParametersType).
			Msg("error while waiting for peer command")

		// error was not fatal for the FSM, jump back to the connect state.
		// but first send an error to the peer.
		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: cmdErr.Error(),
		}

		return (*Server).connect, nil
	}

	s.PeerRespOut <- &msg.Message{
		Type: msg.ServerParametersType,
		Value: &msg.ServerParametersResponse{
			//XXX hardcoding a value here for now. This is temporary.
			OpenperfURL: "http://localhost:9000",
		},
	}

	cmd, cmdErr = s.waitForPeerCommand(msg.SetConfigType)
	if cmdErr != nil {
		if cmd == nil {
			s.Logger.Error().
				Err(cmdErr).
				Msg("fatal server FSM error. terminating.")

			return nil, cmdErr
		}

		s.Logger.Error().
			Err(cmdErr).
			Str("expected command type", msg.SetConfigType).
			Msg("error while waiting for peer command")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: cmdErr.Error(),
		}

		return (*Server).connect, nil
	}

	// Sanity check our inputs. The more we can validate here the less we'd have to
	// clean up from Openperf if it all goes wrong.
	serverCfg, ok := cmd.Value.(*msg.ServerConfiguration)
	if !ok {
		s.Logger.Error().
			Interface("actual type", cmd.Value).
			Interface("expected type", &msg.ServerConfiguration{}).
			Msg("unexpected peer message value")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: "unexpected peer message value",
		}

		return (*Server).connect, nil
	}

	if err := s.validateTestConfig(serverCfg); err != nil {
		s.Logger.Error().
			Err(err).
			Msg("error with test configuration")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: err,
		}

		return (*Server).connect, nil
	}

	s.TestConfiguration.UpstreamRateBps = serverCfg.UpstreamRateBps
	s.TestConfiguration.DownstreamRateBps = serverCfg.DownstreamRateBps

	//XXX: local Openperf will be configured here.

	//From here on out any error must jump to the cleanup state before exiting or restarting.

	// Send ACK to the client
	s.PeerRespOut <- &msg.Message{
		Type: msg.AckType,
	}

	// Switch to Ready state.
	return (*Server).ready, nil
}

func (s *Server) ready(ctx context.Context) (serverStateFunc, error) {
	s.enter("ready")

	cmd, cmdErr := s.waitForPeerCommand(msg.StartCommandType)
	if cmdErr != nil {
		if cmd == nil {
			return (*Server).cleanup, cmdErr
		}

		s.Logger.Error().
			Err(cmdErr).
			Str("expected command type", msg.StartCommandType).
			Msg("error while waiting for peer command")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: cmdErr.Error(),
		}

		return (*Server).cleanup, nil
	}

	startCmd, ok := cmd.Value.(*msg.StartCommand)
	if !ok {
		s.Logger.Error().
			Interface("actual type", cmd.Value).
			Interface("expected type", &msg.StartCommand{}).
			Msg("unexpected peer message value")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: "unexpected peer message value",
		}

		return (*Server).cleanup, nil
	}

	startTime, err := time.Parse(TimeFormatString, startCmd.StartTime)
	if err != nil {
		s.Logger.Error().
			Err(err).
			Msg("error parsing test start time")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: err.Error(),
		}

		return (*Server).cleanup, nil
	}

	// Is start time in the past?
	if startTime.Before(time.Now()) {
		s.Logger.Error().
			Time("requested start time", startTime).
			Time("now", time.Now()).
			Msg("requested start time is in the past")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: "requested start time is in the past",
		}

		return (*Server).cleanup, nil
	}

	// How about too far in the future?
	if !startTime.Before(time.Now().Add(MaximumStartTimeDelta)) {
		s.Logger.Error().
			Time("requested start time", startTime).
			Time("now", time.Now()).
			Dur("maximum delta", MaximumStartTimeDelta).
			Msg("requested start time is too far in the future")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: "requested start time is too far in the future",
		}

		return (*Server).cleanup, nil
	}

	s.startTime = startTime

	// Send ACK to the client
	s.PeerRespOut <- &msg.Message{
		Type: msg.AckType,
	}

	// switch to armed state
	return (*Server).armed, nil
}

// armed FSM state that waits for the start time to arrive before switching to running state.
func (s *Server) armed(ctx context.Context) (serverStateFunc, error) {
	s.enter("armed")

	// Wait for then to become now (aka test to start.)
	// Also make sure peer didn't disappear on us.
Done:
	for {
		select {
		case <-time.After(time.Until(s.startTime)):
			break Done

		// There's a chance our peer will try to tell us something while we're waiting.
		case notif, ok := <-s.PeerCmdIn:
			if !ok {
				s.Logger.Error().Msg("error reading peer command channel")
				return (*Server).cleanup, &PeerError{What: "error reading peer command."}
			}

			switch notif.Type {

			case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
				s.Logger.Error().
					Msg("unexpected peer disconnection")

				return (*Server).cleanup, nil

			default:
				s.Logger.Error().
					Str("command type", notif.Type).
					Msg("unexpected command from peer while waiting for test to start")

				return (*Server).cleanup, nil

			}

		}
	}

	return (*Server).running, nil
}

// running tracks FSM state where data stream(s) are transmitting (aka test is running.)
func (s *Server) running(ctx context.Context) (serverStateFunc, error) {
	s.enter("running")

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
	case s.TestConfiguration.UpstreamRateBps > 0 && s.TestConfiguration.DownstreamRateBps > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, s.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, s.StatsPollInterval)

		defer txStatsPollCancel()

		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, s.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = true
		serverRunning = true

	// server -> client traffic
	case s.TestConfiguration.DownstreamRateBps > 0:
		// Start up generator polling (to check runstate)
		generatorPollResp, generatorPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetGeneratorRequest{}}, s.GeneratorPollInterval)

		defer generatorPollCancel()

		// Start up transmit stats polling
		txStatsPollResp, txStatsPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetTxStatsRequest{}}, s.StatsPollInterval)

		defer txStatsPollCancel()

		clientRunning = false
		serverRunning = true

	// client -> server traffic
	case s.TestConfiguration.UpstreamRateBps > 0:
		// Start up receive stats polling
		rxStatsPollResp, rxStatsPollCancel = s.startOpenperfCmdRepeater(ctx, &openperf.Command{Request: &openperf.GetRxStatsRequest{}}, s.StatsPollInterval)

		defer rxStatsPollCancel()

		clientRunning = true
		serverRunning = false
	}

Done:
	for {
		select {
		case cmd, ok := <-s.PeerCmdIn:
			if !ok {
				s.Logger.Error().Msg("error reading from peer command channel.")
				return (*Server).cleanup, &PeerError{Err: errors.New("error reading peer notifications.")}
			}

			switch cmd.Type {
			case msg.TransmitDoneType:

				clientRunning = false
				rxStatsPollCancel()

				if !serverRunning {
					break Done
				}

			case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
				// Return an error here. Peer should not have disconnected.
				//XXX: this will be updated when the low level peer interface is implemented.
				s.Logger.Error().Msg("unexpected peer disconnection")
				return (*Server).cleanup, nil

			default:
				s.Logger.Error().
					Str("actual type", cmd.Type).
					Str("expected type", "TransmitDoneType").
					Msg("got unexpected message from peer. Connection terminated.")

				return (*Server).cleanup, nil
			}

		case txStat, ok := <-txStatsPollResp:
			if !ok {
				txStatsPollResp = nil
				continue
			}

			switch stats := txStat.(type) {
			case *openperf.GetTxStatsResponse:
				s.PeerNotifOut <- &msg.Message{
					Type: msg.StatsNotificationType,
					Value: &msg.DataStreamStats{
						TxStats: stats,
					},
				}

			case error:
				s.Logger.Error().
					Err(stats).
					Msg("error occurred while polling Openperf transmit stats.")
				return (*Server).cleanup, nil

			default:
				s.Logger.Error().
					Interface("received", txStat).
					Interface("expected", &openperf.GetTxStatsResponse{}).
					Msg("got an unexpected message type while polling Openperf transmit stats")

				return (*Server).cleanup, &OpenperfError{
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
				s.PeerNotifOut <- &msg.Message{
					Type: msg.StatsNotificationType,
					Value: &msg.DataStreamStats{
						RxStats: stats,
					},
				}

			case error:
				s.Logger.Error().
					Err(stats).
					Msg("error occurred while polling Openperf receive stats.")
				return (*Server).cleanup, nil

			default:
				s.Logger.Error().
					Interface("received", rxStat).
					Interface("expected", &openperf.GetRxStatsResponse{}).
					Msg("got an unexpected message type while polling Openperf receive stats")

				return (*Server).cleanup, &OpenperfError{
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

				if !serverRunning {
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
						s.PeerNotifOut <- &msg.Message{
							Type: msg.StatsNotificationType,
							Value: &msg.DataStreamStats{
								TxStats: stat,
							},
						}

					case error:
						s.Logger.Error().
							Err(stat).
							Msg("error occurred while polling Openperf transmit stats.")
						return (*Server).cleanup, nil

					default:
						s.Logger.Error().
							Interface("received", lastStat).
							Interface("expected", &openperf.GetTxStatsResponse{}).
							Msg("got an unexpected message type while polling Openperf transmit stats")

						return (*Server).cleanup, &OpenperfError{
							Actual:   fmt.Sprintf("%T", lastStat),
							Expected: "openperf.GetTxStatsResponse",
							What:     "sent an openperf.GetTxStatsRequest and got an unexpected response. ",
						}

					}
				}

				// Tell client we're done transmitting.
				s.PeerNotifOut <- &msg.Message{
					Type: msg.TransmitDoneType,
				}

				serverRunning = false
				if !clientRunning {
					break Done
				}

			case error:
				s.Logger.Error().
					Err(generator).
					Msg("error occurred while polling Openperf generator.")
				return (*Server).cleanup, nil

			default:
				s.Logger.Error().
					Interface("received", generator).
					Interface("expected", &openperf.GetGeneratorResponse{}).
					Msg("got an unexpected message type while polling Openperf generator")

				return (*Server).cleanup, &OpenperfError{
					Actual:   fmt.Sprintf("%T", gen),
					Expected: "openperf.GetGeneratorResponse",
					What:     "sent an openperf.GetGeneratorRequest and got an unexpected response. ",
				}
			}
		}
	}

	return (*Server).done, nil
}

// done tracks the FSM state immediately after traffic has stopped. Used to collate results.
func (s *Server) done(ctx context.Context) (serverStateFunc, error) {
	s.enter("done")

	finalStatsValue := &msg.FinalStats{}

	//Get Tx results
	if s.TestConfiguration.DownstreamRateBps > 0 {
		stats, err := s.getFinalServerTxStats(ctx)

		if err != nil {
			s.Logger.Error().Err(err).Msg("error getting final transmit stats")
			return (*Server).cleanup, nil
		}
		finalStatsValue.TxStats = stats
	}

	//Get Rx results
	if s.TestConfiguration.UpstreamRateBps > 0 {
		stats, err := s.getFinalServerRxStats(ctx)

		if err != nil {
			s.Logger.Error().Err(err).Msg("error getting final receive stats")
			return (*Server).cleanup, nil
		}
		finalStatsValue.RxStats = stats
	}

	//Wait for get final results command.
	cmd, err := s.waitForPeerCommand(msg.GetFinalStatsType)
	if err != nil {
		if cmd == nil {
			return (*Server).cleanup, err
		}

		s.Logger.Error().
			Err(err).
			Str("expected command type", msg.GetFinalStatsType).
			Msg("error while waiting for peer command")

		s.PeerRespOut <- &msg.Message{
			Type:  msg.ErrorType,
			Value: err.Error(),
		}

		return (*Server).cleanup, nil
	}

	//Send response.
	s.PeerRespOut <- &msg.Message{
		Type:  msg.FinalStatsType,
		Value: finalStatsValue,
	}

	return (*Server).cleanup, nil
}

func (s *Server) cleanup(ctx context.Context) (serverStateFunc, error) {
	s.enter("cleanup")

	if s.TestConfiguration.DownstreamRateBps > 0 {
		//Delete the generator we created.
		reqDone := make(chan struct{})
		req := &openperf.Command{
			Request: &openperf.DeleteGeneratorRequest{
				Id: "Generator-one"}, //XXX: hardcoded for now as a placeholder.
			Done: reqDone,
			Ctx:  ctx,
		}

		s.OpenperfCmdOut <- req

		// Wait for request to finish one way or another. OP controller ensures this never gets stuck.
		<-reqDone

		//XXX: this is used for unit testing right now. Once the Openperf interface
		// is ready this is likely to change.
		if req.Response != nil {
			return (*Server).error, &OpenperfError{Err: req.Response.(error)}
		}

	}

	if s.errorAfterCleanup {
		return (*Server).error, nil
	}

	return (*Server).connect, nil
}

// error tracks state where Server FSM has to terminate due to an error.
func (s *Server) error(ctx context.Context) (serverStateFunc, error) {
	s.enter("error")

	s.Logger.Error().Msg("Server FSM exiting due to error")

	return nil, nil
}

func (s *Server) validateTestConfig(cfg *msg.ServerConfiguration) error {

	if cfg.UpstreamRateBps == 0 && cfg.DownstreamRateBps == 0 {
		return errors.New("test has upstream and downstream rates of zero")
	}

	return nil
}

// waitForPeerCommand waits for either a command from our peer or a timeout.
// on successful receive of the expected type, return <message, nil>.
// on non-fatal error condition, returns <message, error>.
// on fatal error condition, returns <nil, error>.
func (s *Server) waitForPeerCommand(expectedType string) (*msg.Message, error) {

	select {
	case cmd, ok := <-s.PeerCmdIn:
		if !ok {
			s.Logger.Error().
				Str("expected command type", expectedType).
				Msg("error reading from peer command channel.")

			return nil, &PeerError{What: "error reading from peer command channel."}
		}

		switch cmd.Type {
		case expectedType:
			s.Logger.Trace().
				Str("message type", cmd.Type).
				Msg("received message from peer")

			return cmd, nil

		case msg.ErrorType:
			val, ok := cmd.Value.(error)
			if !ok {
				s.Logger.Error().
					Msg("received error command with unexpected content from peer")

				return cmd, &PeerError{}
			}
			s.Logger.Error().
				Str("error", val.Error()).
				Msg("received error command from peer")

			return cmd, &PeerError{Err: val}

		case msg.PeerDisconnectLocalType, msg.PeerDisconnectRemoteType:
			//XXX this will be updated once the lower-level peer interface is implemented.
			return nil, &PeerError{}
		}

		s.Logger.Error().
			Str("actual", cmd.Type).
			Str("expected", expectedType).
			Msg("unexpected command from peer")

		return cmd, &PeerError{
			What:     "unexpected command from peer",
			Actual:   cmd.Type,
			Expected: expectedType}

	case <-time.After(s.PeerTimeout):
		s.Logger.Error().
			Str("expected command", expectedType).
			Msg("timed out waiting for peer command")

		return &msg.Message{}, &TimeoutError{Operation: "waiting for a reply from peer."}
	}
}

// getFinalServerTxStats gets the final set of server Tx stats after the test has completed.
// returns results, nil on success and nil, error otherwise.
// Errors from this method receiver are not considered fatal to the FSM.
func (s *Server) getFinalServerTxStats(ctx context.Context) (*openperf.GetTxStatsResponse, error) {

	// Do we need to get TxStats?
	if s.TestConfiguration.DownstreamRateBps <= 0 {
		return nil, nil
	}

	reqDone := make(chan struct{})
	opCtx, opCancel := context.WithTimeout(ctx, s.OpenperfTimeout)
	defer opCancel()

	req := &openperf.Command{
		Request: &openperf.GetTxStatsRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	s.OpenperfCmdOut <- req

	<-reqDone

	switch resp := req.Response.(type) {
	case *openperf.GetTxStatsResponse:
		return resp, nil

	case error:
		return nil, resp

	default:
		return nil, &OpenperfError{
			Actual:   fmt.Sprintf("%T", req.Response),
			Expected: "openperf.GetTxStatsResponse",
			What:     "sent an openperf.GetTxStatsRequest and got an unexpected response. ",
		}
	}

}

func (s *Server) getFinalServerRxStats(ctx context.Context) (*openperf.GetRxStatsResponse, error) {

	// Do we need to get RxStats?
	if s.TestConfiguration.UpstreamRateBps <= 0 {
		return nil, nil
	}

	reqDone := make(chan struct{})
	opCtx, opCancel := context.WithTimeout(ctx, s.OpenperfTimeout)
	defer opCancel()

	req := &openperf.Command{
		Request: &openperf.GetRxStatsRequest{},
		Done:    reqDone,
		Ctx:     opCtx,
	}

	s.OpenperfCmdOut <- req

	<-reqDone

	switch resp := req.Response.(type) {
	case *openperf.GetRxStatsResponse:
		return resp, nil

	case error:
		return nil, resp

	default:
		return nil, &OpenperfError{
			Actual:   fmt.Sprintf("%T", req.Response),
			Expected: "openperf.GetRxStatsResponse",
			What:     "sent an openperf.GetRxStatsRequest and got an unexpected response. ",
		}
	}
}

func (s *Server) startOpenperfCmdRepeater(ctx context.Context, cmd *openperf.Command, interval time.Duration) (responses chan interface{}, cancelFn context.CancelFunc) {
	var requestCtx context.Context
	requestCtx, cancelFn = context.WithCancel(ctx)
	responses = make(chan interface{})
	repeater := &openperf.CommandRepeater{
		Command:        cmd,
		Interval:       interval,
		OpenperfCmdOut: s.OpenperfCmdOut,
		Responses:      responses}

	go repeater.Run(requestCtx)

	return

}
