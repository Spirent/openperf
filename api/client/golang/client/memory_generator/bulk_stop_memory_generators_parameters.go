// Code generated by go-swagger; DO NOT EDIT.

package memory_generator

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

// NewBulkStopMemoryGeneratorsParams creates a new BulkStopMemoryGeneratorsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewBulkStopMemoryGeneratorsParams() *BulkStopMemoryGeneratorsParams {
	return &BulkStopMemoryGeneratorsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewBulkStopMemoryGeneratorsParamsWithTimeout creates a new BulkStopMemoryGeneratorsParams object
// with the ability to set a timeout on a request.
func NewBulkStopMemoryGeneratorsParamsWithTimeout(timeout time.Duration) *BulkStopMemoryGeneratorsParams {
	return &BulkStopMemoryGeneratorsParams{
		timeout: timeout,
	}
}

// NewBulkStopMemoryGeneratorsParamsWithContext creates a new BulkStopMemoryGeneratorsParams object
// with the ability to set a context for a request.
func NewBulkStopMemoryGeneratorsParamsWithContext(ctx context.Context) *BulkStopMemoryGeneratorsParams {
	return &BulkStopMemoryGeneratorsParams{
		Context: ctx,
	}
}

// NewBulkStopMemoryGeneratorsParamsWithHTTPClient creates a new BulkStopMemoryGeneratorsParams object
// with the ability to set a custom HTTPClient for a request.
func NewBulkStopMemoryGeneratorsParamsWithHTTPClient(client *http.Client) *BulkStopMemoryGeneratorsParams {
	return &BulkStopMemoryGeneratorsParams{
		HTTPClient: client,
	}
}

/* BulkStopMemoryGeneratorsParams contains all the parameters to send to the API endpoint
   for the bulk stop memory generators operation.

   Typically these are written to a http.Request.
*/
type BulkStopMemoryGeneratorsParams struct {

	/* BulkStop.

	   Bulk stop
	*/
	BulkStop BulkStopMemoryGeneratorsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the bulk stop memory generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *BulkStopMemoryGeneratorsParams) WithDefaults() *BulkStopMemoryGeneratorsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the bulk stop memory generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *BulkStopMemoryGeneratorsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) WithTimeout(timeout time.Duration) *BulkStopMemoryGeneratorsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) WithContext(ctx context.Context) *BulkStopMemoryGeneratorsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) WithHTTPClient(client *http.Client) *BulkStopMemoryGeneratorsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithBulkStop adds the bulkStop to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) WithBulkStop(bulkStop BulkStopMemoryGeneratorsBody) *BulkStopMemoryGeneratorsParams {
	o.SetBulkStop(bulkStop)
	return o
}

// SetBulkStop adds the bulkStop to the bulk stop memory generators params
func (o *BulkStopMemoryGeneratorsParams) SetBulkStop(bulkStop BulkStopMemoryGeneratorsBody) {
	o.BulkStop = bulkStop
}

// WriteToRequest writes these params to a swagger request
func (o *BulkStopMemoryGeneratorsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if err := r.SetBodyParam(o.BulkStop); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
