package openperf

import (
	"context"
	"errors"
	"fmt"
	"time"
)

// MinimumInterval shortest time allowed between CommandRepeater commands to Openperf.
// This value is somewhat arbitrary, but is required so callers can reasonably cancel
// the Run() function.
const MinimumInterval = 10 * time.Millisecond

// CommandRepeater repeatedly send the same Command to an Openperf instance.
// This should only ever be used for GET requests.
type CommandRepeater struct {
	// Cmd command to repeatedly send. Must be allocated by the client and must not be nil.
	Command *Command

	// Interval between sending Cmd to Openperf.
	Interval time.Duration

	// Responses channel relays responses to the caller. This channel must be provided
	// by the caller and must not be nil. This channel is closed when RunCommandRepeater() returns.
	Responses chan interface{}

	// OpenperfController channel to send Command to Openperf on. RequestRepeater will never
	// close this channel.
	OpenperfController chan<- *Command
}

// RunRequestRepeater transmit Request every Interval until canceled by ctx.
func RunCommandRepeater(ctx context.Context, cr *CommandRepeater) error {

	if cr == nil {
		return errors.New("Cannot pass nil CommandRepeater to Run")
	}

	if cr.Command == nil {
		return errors.New("CommandRepeater.Command must not be nil")
	}

	if cr.OpenperfController == nil {
		return errors.New("CommandRepeater.OpenperfController must not be nil")
	}

	if cr.Interval < MinimumInterval {
		return fmt.Errorf("CommandRepeater.Interval must be >= %dms", MinimumInterval.Milliseconds())
	}

	if cr.Responses == nil {
		return errors.New("CommandRepeater.Responses must not be nil")
	}

	cr.Command.Ctx = ctx

Done:
	for {
		select {
		case <-ctx.Done():
			break Done

		case <-time.After(cr.Interval):
			cmd := cr.Command

			// Populate the relevant fields of Request
			cmd.Done = make(chan struct{})

			// Send Request to Openperf.
			cr.OpenperfController <- cmd

			// Wait for Openperf to signal request done and response ready.
			<-cmd.Done

			// Send response to the caller.
			cr.Responses <- cmd.Response
		}
	}

	close(cr.Responses)

	return nil
}
