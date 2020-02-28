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

// InvalidConfigurationError returned when a test configuration contains invalid parameters.
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

// InvalidParamError returned when FSM has incorrect parameter value.
type InvalidParamError struct {
	//What details of the parameter.
	What string
	//Actual invalid parameter value.
	Actual string
	//Expected valid parameter value or details.
	Expected string

	//Err optional child error.
	Err error
}

func (e *InvalidParamError) Error() string {
	return fmt.Sprintf("FSM parameter %s has invalid value %s. expected %s", e.What, e.Actual, e.Expected)
}

// OpenperfError returned when there's an issue with Openperf.
type OpenperfError struct {
	//What additional error information.
	What string

	//Actual value. Optional.
	Actual string

	//Expected value.
	Expected string

	//Err optional child error.
	Err error
}

func (e *OpenperfError) Error() string {
	message := " an error occurred with Openperf "

	if e.What != "" {
		message += e.What + " "
	}

	if e.Expected != "" {
		message += "expected: " + e.Expected + " "
	}

	if e.Actual != "" {
		message += "actual: " + e.Actual + ". "
	}

	if e.Err != nil {
		message += e.Err.Error()
	}

	return message
}

// PeerError returned when there's an issue with our peer.
type PeerError struct {
	//What additional error information.
	What string

	//Actual value. Optional.
	Actual string

	//Expected value.
	Expected string

	//Err optional child error.
	Err error
}

func (e *PeerError) Error() string {
	message := "an error occurred with peer "

	if e.What != "" {
		message += e.What + " "
	}

	if e.Expected != "" {
		message += "expected: " + e.Expected + " "
	}

	if e.Actual != "" {
		message += "actual: " + e.Actual + ". "
	}

	if e.Err != nil {
		message += e.Err.Error()
	}

	return message
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
