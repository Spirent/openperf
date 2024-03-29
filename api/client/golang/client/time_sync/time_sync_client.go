// Code generated by go-swagger; DO NOT EDIT.

package time_sync

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"
)

// New creates a new time sync API client.
func New(transport runtime.ClientTransport, formats strfmt.Registry) ClientService {
	return &Client{transport: transport, formats: formats}
}

/*
Client for time sync API
*/
type Client struct {
	transport runtime.ClientTransport
	formats   strfmt.Registry
}

// ClientOption is the option for Client methods
type ClientOption func(*runtime.ClientOperation)

// ClientService is the interface for Client methods
type ClientService interface {
	CreateTimeSource(params *CreateTimeSourceParams, opts ...ClientOption) (*CreateTimeSourceCreated, error)

	DeleteTimeSource(params *DeleteTimeSourceParams, opts ...ClientOption) (*DeleteTimeSourceNoContent, error)

	GetTimeCounter(params *GetTimeCounterParams, opts ...ClientOption) (*GetTimeCounterOK, error)

	GetTimeKeeper(params *GetTimeKeeperParams, opts ...ClientOption) (*GetTimeKeeperOK, error)

	GetTimeSource(params *GetTimeSourceParams, opts ...ClientOption) (*GetTimeSourceOK, error)

	ListTimeCounters(params *ListTimeCountersParams, opts ...ClientOption) (*ListTimeCountersOK, error)

	ListTimeSources(params *ListTimeSourcesParams, opts ...ClientOption) (*ListTimeSourcesOK, error)

	SetTransport(transport runtime.ClientTransport)
}

/*
  CreateTimeSource registers a time source for time syncing

  Registers a new time source for time syncing. Time sources are
immutable.

*/
func (a *Client) CreateTimeSource(params *CreateTimeSourceParams, opts ...ClientOption) (*CreateTimeSourceCreated, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewCreateTimeSourceParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "CreateTimeSource",
		Method:             "POST",
		PathPattern:        "/time-sources",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &CreateTimeSourceReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*CreateTimeSourceCreated)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for CreateTimeSource: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  DeleteTimeSource deletes a time source

  Deletes an existing time source. Idempotent.
*/
func (a *Client) DeleteTimeSource(params *DeleteTimeSourceParams, opts ...ClientOption) (*DeleteTimeSourceNoContent, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewDeleteTimeSourceParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "DeleteTimeSource",
		Method:             "DELETE",
		PathPattern:        "/time-sources/{id}",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &DeleteTimeSourceReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*DeleteTimeSourceNoContent)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for DeleteTimeSource: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  GetTimeCounter gets a time counter

  Returns a time counter, by id.
*/
func (a *Client) GetTimeCounter(params *GetTimeCounterParams, opts ...ClientOption) (*GetTimeCounterOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewGetTimeCounterParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "GetTimeCounter",
		Method:             "GET",
		PathPattern:        "/time-counters/{id}",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &GetTimeCounterReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*GetTimeCounterOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for GetTimeCounter: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  GetTimeKeeper gets a time keeper

  Returns the system time keeper, aka clock.
*/
func (a *Client) GetTimeKeeper(params *GetTimeKeeperParams, opts ...ClientOption) (*GetTimeKeeperOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewGetTimeKeeperParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "GetTimeKeeper",
		Method:             "GET",
		PathPattern:        "/time-keeper",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &GetTimeKeeperReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*GetTimeKeeperOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for GetTimeKeeper: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  GetTimeSource gets a time source

  Get a time source, by id.
*/
func (a *Client) GetTimeSource(params *GetTimeSourceParams, opts ...ClientOption) (*GetTimeSourceOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewGetTimeSourceParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "GetTimeSource",
		Method:             "GET",
		PathPattern:        "/time-sources/{id}",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &GetTimeSourceReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*GetTimeSourceOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for GetTimeSource: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  ListTimeCounters lists time counters

  The `time-counters` endpoint returns all local time counters
that are available for measuring the passage of time.  This
list is for informational purposes only.

*/
func (a *Client) ListTimeCounters(params *ListTimeCountersParams, opts ...ClientOption) (*ListTimeCountersOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewListTimeCountersParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "ListTimeCounters",
		Method:             "GET",
		PathPattern:        "/time-counters",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &ListTimeCountersReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*ListTimeCountersOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for ListTimeCounters: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

/*
  ListTimeSources lists reference clocks

  The `time-sources` endpoint returns all time sources
that are used for syncing the local time.

*/
func (a *Client) ListTimeSources(params *ListTimeSourcesParams, opts ...ClientOption) (*ListTimeSourcesOK, error) {
	// TODO: Validate the params before sending
	if params == nil {
		params = NewListTimeSourcesParams()
	}
	op := &runtime.ClientOperation{
		ID:                 "ListTimeSources",
		Method:             "GET",
		PathPattern:        "/time-sources",
		ProducesMediaTypes: []string{"application/json"},
		ConsumesMediaTypes: []string{"application/json"},
		Schemes:            []string{"http", "https"},
		Params:             params,
		Reader:             &ListTimeSourcesReader{formats: a.formats},
		Context:            params.Context,
		Client:             params.HTTPClient,
	}
	for _, opt := range opts {
		opt(op)
	}

	result, err := a.transport.Submit(op)
	if err != nil {
		return nil, err
	}
	success, ok := result.(*ListTimeSourcesOK)
	if ok {
		return success, nil
	}
	// unexpected success response
	// safeguard: normally, absent a default response, unknown success responses return an error above: so this is a codegen issue
	msg := fmt.Sprintf("unexpected success response for ListTimeSources: API contract not enforced by server. Client expected to get an error, but got: %T", result)
	panic(msg)
}

// SetTransport changes the transport on the client
func (a *Client) SetTransport(transport runtime.ClientTransport) {
	a.transport = transport
}
