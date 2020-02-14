package fsm

import "time"

const (
	DefaultPeerTimeout           = 30 * time.Second
	DefaultOpenperfTimeout       = 30 * time.Second
	DefaultStartDelay            = 3 * time.Second
	DefaultStatsPollInterval     = 1 * time.Second
	DefaultGeneratorPollInterval = 1 * time.Second
)
