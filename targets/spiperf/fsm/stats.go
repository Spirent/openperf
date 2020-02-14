package fsm

type StatsKind string

const (
	UpstreamTxKind   = "UpstreamTxKind"
	UpstreamRxKind   = "UpstreamRxKind"
	DownstreamTxKind = "DownstreamTxKind"
	DownstreamRxKind = "DownstreamRxKind"
)

type Stats struct {
	Kind   StatsKind
	Values interface{}
}
