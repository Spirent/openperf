// Code generated by go-swagger; DO NOT EDIT.

package packet_analyzers

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"net/http"
	"time"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	cr "github.com/go-openapi/runtime/client"
	"github.com/go-openapi/strfmt"
)

// NewDeletePacketAnalyzersParams creates a new DeletePacketAnalyzersParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewDeletePacketAnalyzersParams() *DeletePacketAnalyzersParams {
	return &DeletePacketAnalyzersParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewDeletePacketAnalyzersParamsWithTimeout creates a new DeletePacketAnalyzersParams object
// with the ability to set a timeout on a request.
func NewDeletePacketAnalyzersParamsWithTimeout(timeout time.Duration) *DeletePacketAnalyzersParams {
	return &DeletePacketAnalyzersParams{
		timeout: timeout,
	}
}

// NewDeletePacketAnalyzersParamsWithContext creates a new DeletePacketAnalyzersParams object
// with the ability to set a context for a request.
func NewDeletePacketAnalyzersParamsWithContext(ctx context.Context) *DeletePacketAnalyzersParams {
	return &DeletePacketAnalyzersParams{
		Context: ctx,
	}
}

// NewDeletePacketAnalyzersParamsWithHTTPClient creates a new DeletePacketAnalyzersParams object
// with the ability to set a custom HTTPClient for a request.
func NewDeletePacketAnalyzersParamsWithHTTPClient(client *http.Client) *DeletePacketAnalyzersParams {
	return &DeletePacketAnalyzersParams{
		HTTPClient: client,
	}
}

/* DeletePacketAnalyzersParams contains all the parameters to send to the API endpoint
   for the delete packet analyzers operation.

   Typically these are written to a http.Request.
*/
type DeletePacketAnalyzersParams struct {
	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the delete packet analyzers params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeletePacketAnalyzersParams) WithDefaults() *DeletePacketAnalyzersParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the delete packet analyzers params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeletePacketAnalyzersParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) WithTimeout(timeout time.Duration) *DeletePacketAnalyzersParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) WithContext(ctx context.Context) *DeletePacketAnalyzersParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) WithHTTPClient(client *http.Client) *DeletePacketAnalyzersParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the delete packet analyzers params
func (o *DeletePacketAnalyzersParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WriteToRequest writes these params to a swagger request
func (o *DeletePacketAnalyzersParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
