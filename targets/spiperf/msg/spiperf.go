package msg

import "github.com/Spirent/openperf/targets/spiperf/openperf"

// Message type definitions
const (
	AckType                 = "AckType" //No message value needed.
	ErrorType               = "ErrorType"
	HelloType               = "HelloType"
	GetServerParametersType = "GetServerParametersType"
	ServerParametersType    = "ServerParametersType"
	GetConfigType           = "GetConfigType"
	SetConfigType           = "SetConfigType"
	StartCommandType        = "StartCommandType"
	StatsNotificationType   = "StatsNotificationType"
	TransmitDoneType        = "TransmitDoneType"
	GetFinalStatsType       = "GetFinalStatsType"
	FinalStatsType          = "FinalStatsType"
)

// Message is a message envelope for communication between peers.
type Message struct {
	Type  string      `json:"type"`
	Value interface{} `json:"value,omitempty"`
}

// Hello initiates a session between client and server instances.
type Hello struct {
	PeerProtocolVersion string `json:"peer_protocol_version"`
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
	TransmitDuration uint `json:"transmit_duration"`
	FixedFrameSize   uint `json:"fixed_frame_size"`
}

// StartCommand tells the server what time the test starts running.
// At the given time the server will start transmitting and/or receiving packets.
type StartCommand struct {
	StartTime string `json:"start_time"`
}

// RuntimeStats convey stats while test is running.
type RuntimeStats struct {
	TxStats *openperf.GetTxStatsResponse `json:"get_tx_stats,omitempty"`
	RxStats *openperf.GetRxStatsResponse `json:"get_rx_stats,omitempty"`
}

// FinalStats convey final set of stats sampled after the test completes.
type FinalStats struct {
	TransmitFrames uint64 `json:"transmit_frames"`
}
