package fsm

type StatsKind string

const (
	DownstreamTxKind = "DownstreamTxKind"
	DownstreamRxKind = "DownstreamRxKind"
	UpstreamTxKind   = "UpstreamTxKind"
	UpstreamRxKind   = "UpstreamRxKind"
)

type Stats struct {
	Final  bool
	Kind   StatsKind
	Values interface{}
}
