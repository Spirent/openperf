package openperf

import (
	"context"
	"time"
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
	// and a response (or error condition) has returned. After this occurs, the
	// requester may safely check Response. No values are ever sent on this channel.
	// It is provided by the requester, however if the requester doesn't care
	// to synchronize on request completion then Done may be nil.
	Done chan struct{}

	// Response is a pointer to a request-specific result type. It is
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

type DeleteGeneratorRequest struct {
	Id string
}

type GetGeneratorRequest struct {
}

//FIXME: hack. Change to using swagger-generated results object asap.
type GetGeneratorResponse struct {
	Running bool
}

type GetStatsRequest struct {
}

//FIXME: hack. Change to using swagger-generated results object asap.
type GetStatsResponse struct {
	Timestamp uint64
	TxPackets uint64
	TxBytes   uint64
}

type GetTimeRequest struct {
}

//FIXME: hack. Change to using swagger-generated results object asap.
type GetTimeResponse struct {
	SystemTime time.Time
}
