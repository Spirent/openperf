package fsm

import "fmt"

// InternalError returned when a not-otherwise-specified error occurs.
type InternalError struct {
	//Message contains details about the error
	Message string

	//Err optional child error.
	Err error
}

func (e *InternalError) Error() string {
	return "internal error occurred: " + e.Message
}

// InvalidConfigurationError returned when a configuration contains invalid parameters.
type InvalidConfigurationError struct {
	//What details of the configuration parameter.
	What string
	//Actual invalid parameter value.
	Actual string
	//Expected valid parameter value or details.
	Expected string

	//Err optional child error.
	Err error
}

func (e *InvalidConfigurationError) Error() string {
	return fmt.Sprintf("invalid configuration parameter %s. got %s. expected %s", e.What, e.Actual, e.Expected)
}

// InvalidFSMParamError returned when FSM has incorrect parameter value.
type InvalidFSMParamError struct {
	//What details of the parameter.
	What string
	//Actual invalid parameter value.
	Actual string
	//Expected valid parameter value or details.
	Expected string

	//Err optional child error.
	Err error
}

func (e *InvalidFSMParamError) Error() string {
	return fmt.Sprintf("FSM parameter %s has invalid value %s. expected %s", e.What, e.Actual, e.Expected)
}

// MalformedPeerMessageError returned when a peer message Type parameter does not map to the
//expected Value type.
type MalformedPeerMessageError struct {
	//Type msg.Type value
	Type string

	// ActualValueType actual golang type of msg.Value
	ActualValueType string
	// ExpectedValueType expected golang type of msg.Value
	ExpectedValueType string

	//Err optional child error.
	Err error
}

func (e *MalformedPeerMessageError) Error() string {
	return fmt.Sprintf("malformed peer message: for envelope type %s got value of type %s. expected %s", e.Type, e.ActualValueType, e.ExpectedValueType)
}

// MessagingError returned when there is an issue with messaging between peer instances.
type MessagingError struct {
	//Err optional child error.
	Err error
}

func (e *MessagingError) Error() string {
	if e.Err == nil {
		return "unspecified messaging error"
	}

	return "error with messaging: " + e.Err.Error()
}

// OpenperfError returned when peer responds with an error message.
type OpenperfError struct {
	//Err optional child error.
	Err error
}

func (e *OpenperfError) Error() string {
	if e.Err == nil {
		return "unspecified error response from Openperf"
	}

	return "error response from Openperf: " + e.Err.Error()
}

// PeerError returned when peer responds with an error message.
type PeerError struct {
	//Err optional child error.
	Err error
}

func (e *PeerError) Error() string {
	if e.Err == nil {
		return "unspecified error response from peer"
	}

	return "error response from peer: " + e.Err.Error()
}

// TimeoutError returned when an operation times out. Operations include peer and Openperf communication.
type TimeoutError struct {
	//Operation detail regarding what timed out.
	Operation string
	//Err optional child error.
	Err error
}

func (e *TimeoutError) Error() string {
	return "timed out while " + e.Operation
}

// UnexpectedOpenperfRespError returned when Openperf sends an unexpected type of response.
type UnexpectedOpenperfRespError struct {
	//Actual response value.
	Actual string
	//Expected response value or details.
	Expected string
	//Request type of request that yielded the unexpected response
	Request string

	//Err optional child error.
	Err error
}

func (e *UnexpectedOpenperfRespError) Error() string {
	return fmt.Sprintf("got unexpected response %s from Openperf while processing request %s. expected %s", e.Actual, e.Request, e.Expected)

}

// UnexpectedPeerDisconnectError returned when peer unexpectedly disconnects.
// Disconnect can be remote (i.e. peer tells us it cannot continue) or
// local (i.e. TCP connection closes or JSON encode/decode error.)
type UnexpectedPeerDisconnectError struct {

	//Local true if the source of the disconnect was the local instance, false otherwise.
	Local bool

	//Err optional child error.
	Err error
}

func (e *UnexpectedPeerDisconnectError) Error() string {

	childError := "unspecified error"
	if e.Err != nil {
		childError = e.Err.Error()
	}

	var source string
	if e.Local {
		source = "local"
	} else {
		source = "remote"
	}

	return fmt.Sprintf("unexpected peer disconnect due to %s error: %s", source, childError)
}

// UnexpectedPeerRespError returned when peer sends an unexpected response type.
type UnexpectedPeerRespError struct {
	//Actual response value.
	Actual string
	//Expected response value or details.
	Expected string

	//Err optional child error.
	Err error
}

func (e *UnexpectedPeerRespError) Error() string {
	return fmt.Sprintf("received an unexpected response from peer: %s. Expected: %s.", e.Actual, e.Expected)
}

// VersionMismatchError returned when a versioned message set differs between client and server.
type VersionMismatchError struct {
	//Actual found version.
	Actual string
	//Expected expected version.
	Expected string
	//What describes what version mismatch is between.
	What string

	//Err optional child error.
	Err error
}

func (e *VersionMismatchError) Error() string {
	return fmt.Sprintf("version mismatch between %s. found %s, expected %s.", e.What, e.Actual, e.Expected)
}
