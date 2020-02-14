package fsm

// InternalError returned when a not-otherwise-specified error occurs.
type InternalError struct {
	Message string
	Err     error
}

func (e *InternalError) Error() string {
	return "internal error occurred: " + e.Message //+ ": " + e.Err.Error()
}

// InvalidConfigurationError returned when a configuration contains invalid parameters.
type InvalidConfigurationError struct {
	Message string
	Err     error
}

func (e *InvalidConfigurationError) Error() string {
	return "invalid configuration: " + e.Message //+ ": " + e.Err.Error()
}

// MessagingError returned when there is an issue with messaging between peer spiperf instances.
type MessagingError struct {
	Message string
	Err     error
}

func (e *MessagingError) Error() string {
	return "messaging error occurred: " + e.Message //+ ": " + e.Err.Error()
}

// PeerError returned when peer responds with an error message.
type OpenperfError struct {
	Message string
	Err     error
}

func (e *OpenperfError) Error() string {
	return "error response from Openperf: " + e.Message
}

// PeerError returned when peer responds with an error message.
type PeerError struct {
	Message string
	Err     error
}

func (e *PeerError) Error() string {
	return "error response from peer: " + e.Message
}

// TimeoutError returned when an operation times out. Operations include peer and Openperf communication.
type TimeoutError struct {
	Message string
	Err     error
}

func (e *TimeoutError) Error() string {
	return "operation timed out: " + e.Message //+ ": " + e.Err.Error()
}

// UnexpectedOpenperfRespError returned when Openperf sends an unexpected type of response.
type UnexpectedOpenperfRespError struct {
	Message string
	Err     error
}

func (e *UnexpectedOpenperfRespError) Error() string {
	return "unexpected Openperf response error: " + e.Message

}

// UnexpectedPeerRespError returned when peer sends an unexpected response type.
type UnexpectedPeerRespError struct {
	Message string
	Err     error
}

func (e *UnexpectedPeerRespError) Error() string {
	return "unexpected peer response error: " + e.Message

}

// VersionMismatchError returned when a versioned message set differs between client and server.
type VersionMismatchError struct {
	Message string
	Err     error
}

func (e *VersionMismatchError) Error() string {
	return "version mismatch: " + e.Message
}
