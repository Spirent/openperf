package openperf

import (
	"context"

	"github.com/Spirent/openperf/targets/spiperf/openperf/v1/models"
)

// Command encapsulates an Openperf REST API request and reply.
type Command struct {
	// Request is a pointer to a specific request type. It must be allocated
	// by the requester.
	Request interface{}

	// Ctx is a per-command context that may be used to limit the lifetime
	// of the request. If this context is ever prematurely canceled or times
	// out then the Openperf Controller may abort processing of the request.
	// It must be provided by the requester.
	Ctx context.Context

	// Done is closed by the Openperf Controller after the request has been sent
	// and a response (or error) has returned. After this occurs, the
	// requester may safely check Response. No values are ever sent on this channel.
	// It is provided by the requester, however if the requester doesn't care
	// to synchronize on request completion then Done may be nil.
	Done chan struct{}

	// Response is a pointer to a request-specific result type or an error. It is
	// allocated by the Openperf Controller and should only be read by
	// the requester after the Done channel is closed (or the requester risks a data race).
	Response interface{}
}

func (c *Command) SignalDone(res interface{}) {
	c.Response = res
	if c.Done != nil {
		close(c.Done)
	}
}

// DeleteGeneratorRequest request Openperf to delete specified generator resource.
type DeleteGeneratorRequest struct {
	Id string
}

// GetGeneratorRequest request Openperf to return configured generator resource.
type GetGeneratorRequest struct {
	Id string
}

// GetGeneratorResponse response from Openperf containing the requested generator resource.
//XXX: placeholder until the Openperf generator API is ready.
type GetGeneratorResponse struct {
	Running bool
}

// GetRxStatsRequest request Openperf to return relevant receive statistics.
type GetRxStatsRequest struct {
}

// GetRxStatsResponse response from Openperf containing requested receive statistics.
//XXX: most probably a placeholder until the Openperf analyzer API is ready.
type GetRxStatsResponse struct {
	Timestamp string
	TxPackets uint64
	TxBytes   uint64
}

// GetTimeRequest request Openperf instance current time.
type GetTimeRequest struct {
}

type GetTimeResponse models.TimeKeeper

// GetTxStatsRequest request Openperf to return relevant transmit statistics.
type GetTxStatsRequest struct {
}

// GetTxStatsResponse response containing requested transmit statistics.
//XXX: most probably a placeholder until the Openperf generator API is ready.
type GetTxStatsResponse struct {
	Timestamp string
	TxPackets uint64
	TxBytes   uint64
}
