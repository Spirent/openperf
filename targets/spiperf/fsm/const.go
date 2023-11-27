package fsm

import "time"

const (
	DefaultPeerTimeout           = 30 * time.Second
	DefaultOpenperfTimeout       = 30 * time.Second
	DefaultStartDelay            = 3 * time.Second
	DefaultStatsPollInterval     = 1 * time.Second
	DefaultGeneratorPollInterval = 1 * time.Second
	TimeFormatString             = time.RFC3339Nano

	// MaximumStartTimeDelta maximum time in the future to start traffic.
	// This value is somewhat arbitrary.
	MaximumStartTimeDelta = 3 * time.Minute
)
