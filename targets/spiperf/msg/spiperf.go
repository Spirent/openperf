package msg

import "github.com/Spirent/openperf/targets/spiperf/openperf"

// Message type definitions. These are used as the Type parameter for
// Message struct below.
const (
	AckType                  = "AckType" //No associated message Value struct.
	ErrorType                = "ErrorType"
	FinalStatsType           = "FinalStatsType"
	GetConfigType            = "GetConfigType"
	GetFinalStatsType        = "GetFinalStatsType" //No associated message Value struct.
	GetServerParametersType  = "GetServerParametersType"
	HelloType                = "HelloType"
	PeerDisconnectLocalType  = "PeerDisconnectLocalType"
	PeerDisconnectRemoteType = "PeerDisconnectRemoteType"
	ServerParametersType     = "ServerParametersType"
	SetConfigType            = "SetConfigType"
	StartCommandType         = "StartCommandType"
	StatsNotificationType    = "StatsNotificationType"
	TransmitDoneType         = "TransmitDoneType" //No associated message Value struct.
)

// Message is a message envelope for communication between peers.
// It combines a message type definition from above with a value structure from below.
type Message struct {
	Type  string      `json:"type"`
	Value interface{} `json:"value,omitempty"`
}

// Following structures are used as Value parameters for Message struct.
// Not all message types have an associated struct.

// FinalStats convey final set of stats sampled after the test completes.
type FinalStats DataStreamStats

// Hello initiates a session between client and server instances.
type Hello struct {
	PeerProtocolVersion string `json:"peer_protocol_version"`
}

// PeerDisconnectLocalNotif sent locally when the peer has unexpectedly disconnected/disappeared.
// Optional Err field details any associated error conditions.
// This notification is intended for local use and not between peers.
type PeerDisconnectLocalNotif struct {
	// Err details about what caused the disconnect.
	// This field is intentionally omitted from JSON since it would be empty anyway.
	Err error `json:"-"`
}

// PeerDisconnectRemoteNotif send by peer to indicate it's terminating the connection.
type PeerDisconnectRemoteNotif struct {
	// Err string detailing any error that might have happened.
	Err string `json:"err,omitempty"`
}

// DataStreamStats convey stats while test is running.
type DataStreamStats struct {
	TxStats *openperf.GetTxStatsResponse `json:"get_tx_stats,omitempty"`
	RxStats *openperf.GetRxStatsResponse `json:"get_rx_stats,omitempty"`
}

// ServerConfiguration sends the server's view of the test configuration to the server.
type ServerConfiguration struct {
	TransmitDuration  uint   `json:"transmit_duration"`
	FixedFrameSize    uint   `json:"fixed_frame_size"`
	UpstreamRateBps   uint64 `json:"upstream_rate_bps"`
	DownstreamRateBps uint64 `json:"downstream_rate_bps"`
}

// ServerParametersResponse conveys the server's parameters to the client.
type ServerParametersResponse struct {
	OpenperfURL  string   `json:"openperf_url"`
	LinkSpeed    uint     `json:"link_speed"`
	ProtocolList []string `json:"protocol_list"`
	AddressLlist []string `json:"address_list"`
}

// StartCommand tells the server what time the test starts running.
// At the given time the server will start transmitting and/or receiving packets.
type StartCommand struct {
	StartTime string `json:"start_time"`
}
