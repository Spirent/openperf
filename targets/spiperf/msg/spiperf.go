package msg

import "time"

const (
	AckType                      = "ACK" //No message value needed.
	ErrorType                    = "ERROR"
	HelloType                    = "HELLO"
	GetParametersMessageType     = "GETPARAMETERS"
	ServerParametersResponseType = "SERVERPARAMETERSRESPONSE"
	GetConfigType                = "GETCONFIG"
	SetConfigType                = "SETCONFIG"
	StartCommandType             = "STARTCOMMAND"
	StatsNotificationType        = "STATSNOTIFICATION"
	StopNotificationType         = "STOPNOTIFICATION" //No message value needed.
	TransmitDoneType             = "TRANSMITDONE"
	GetFinalStatsType            = "GETFINALSTATS"
	FinalStatsType               = "FINALSTATS"
	CleanupType                  = "CLEANUP" //No message value needed.
)

// Message is a message envelope for communication between spiperf instances.
type Message struct {
	Type  string      `json:"type"`
	Value interface{} `json:"value,omitempty"`
}

// Hello initiates a session between client and server spiperf instances.
type Hello struct {
	Version string `json:"version"`
}

// ServerParametersResponse conveys the server's parameters to the client.
type ServerParametersResponse struct {
	OpenperfURL  string   `json:"openperf_url"`
	LinkSpeed    uint     `json:"link_speed"`
	ProtocolList []string `json:"protocol_list"`
	AddressLlist []string `json:"address_list"`
}

// ServerConfiguration sends the server's view of the test configuration to the server.
type ServerConfiguration struct {
	TransmitDuration uint
	FixedFrameSize   uint
}

// StartCommand tells the server what time the test starts running.
// At the given time the server will start transmitting and/or receiving packets.
type StartCommand struct {
	StartTime time.Time
}

type RuntimeStats struct {
	Timestamp uint64
}

type FinalStats struct {
	TransmitFrames uint64
}
