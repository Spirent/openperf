package openperf

import (
	"context"
	"errors"
	"fmt"
	"time"
)

// CommandRepeaterMinimumInterval shortest time allowed between CommandRepeater commands
// to Openperf. This value is somewhat arbitrary, but is required so callers
// can reasonably cancel the Run() function.
const CommandRepeaterMinimumInterval = 10 * time.Millisecond

// CommandRepeater repeatedly send the same Command to an Openperf instance.
// This should only ever be used for GET requests.
type CommandRepeater struct {
	// Command what to repeatedly send. Must be allocated by the client and must not be nil.
	Command *Command

	// Interval between sending Cmd to Openperf.
	Interval time.Duration

	// Responses channel relays responses to the caller. This channel must be provided
	// by the caller and must not be nil. This channel is closed when RunCommandRepeater() returns.
	Responses chan interface{}

	// OpenperfCmdOut channel to send Command to Openperf on. CommandRepeater will never
	// close this channel.
	OpenperfCmdOut chan<- *Command
}

// Run transmit Command every Interval until canceled by ctx.
func (cr *CommandRepeater) Run(ctx context.Context) error {

	if cr == nil {
		return errors.New("Cannot pass nil CommandRepeater to Run")
	}

	if cr.Command == nil {
		return errors.New("CommandRepeater.Command must not be nil")
	}

	if cr.OpenperfCmdOut == nil {
		return errors.New("CommandRepeater.OpenperfCmdOut must not be nil")
	}

	if cr.Interval < CommandRepeaterMinimumInterval {
		return fmt.Errorf("CommandRepeater.Interval must be >= %s", CommandRepeaterMinimumInterval)
	}

	if cr.Responses == nil {
		return errors.New("CommandRepeater.Responses must not be nil")
	}

	cr.Command.Ctx = ctx

	defer close(cr.Responses)
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
			cr.OpenperfCmdOut <- cmd

			// Wait for Openperf to signal request done and response ready.
			<-cmd.Done

			// Send response to the caller.
			cr.Responses <- cmd.Response
		}
	}

	return nil
}
