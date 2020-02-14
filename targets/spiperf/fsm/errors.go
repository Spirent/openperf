package fsm

type InvalidCommandError struct {
	Message string
	Err     error
}

func (e *InvalidCommandError) Error() string {
	return "Invalid command: " + e.Message //+ ": " + e.Err.Error()
}

type InvalidConfigurationError struct {
	Message string
	Err     error
}

type InternalError struct {
	Message string
	Err     error
}

type TimeoutError struct {
	Message string
	Err     error
}

type MessagingError struct {
	Message string
	Err     error
}

func (e *InvalidConfigurationError) Error() string {
	return "Invalid configuration: " + e.Message //+ ": " + e.Err.Error()
}

func (e *InternalError) Error() string {
	return "Internal Error Occurred: " + e.Message //+ ": " + e.Err.Error()
}

func (e *TimeoutError) Error() string {
	return "Timeout Error Occurred: " + e.Message //+ ": " + e.Err.Error()
}

func (e *MessagingError) Error() string {
	return "Messaging Error Error Occurred: " + e.Message //+ ": " + e.Err.Error()
}
