package fsm

// InternalError returned when a not-otherwise-specified error occurs.
type InternalError struct {
	Message string
	Err     error
}

func (e *InternalError) Error() string {
	return "Internal Error Occurred: " + e.Message //+ ": " + e.Err.Error()
}

// InvalidConfigurationError returned when a configuration contains invalid parameters.
type InvalidConfigurationError struct {
	Message string
	Err     error
}

func (e *InvalidConfigurationError) Error() string {
	return "Invalid configuration: " + e.Message //+ ": " + e.Err.Error()
}

// MessagingError returned when there is an issue with messaging between peer spiperf instances.
type MessagingError struct {
	Message string
	Err     error
}

func (e *MessagingError) Error() string {
	return "Messaging Error Error Occurred: " + e.Message //+ ": " + e.Err.Error()
}

// PeerError returned when peer responds with an error message.
type PeerError struct {
	Message string
	Err     error
}

func (e *PeerError) Error() string {
	return "Error response from peer: " + e.Message
}

// TimeoutError returned when an operation times out.
type TimeoutError struct {
	Message string
	Err     error
}

func (e *TimeoutError) Error() string {
	return "Operation timed out: " + e.Message //+ ": " + e.Err.Error()
}

// UnexpectedPeerRespError returned when peer sends an unexpected response type.
type UnexpectedPeerRespError struct {
	Message string
	Err     error
}

func (e *UnexpectedPeerRespError) Error() string {
	return "Unexpected peer response error: " + e.Message

}

// VersionMismatchError returned when a versioned message set differs between client and server.
type VersionMismatchError struct {
	Message string
	Err     error
}

func (e *VersionMismatchError) Error() string {
	return "Version mismatch: " + e.Message
}
