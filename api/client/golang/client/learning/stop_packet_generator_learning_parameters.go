// Code generated by go-swagger; DO NOT EDIT.

package learning

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

// NewStopPacketGeneratorLearningParams creates a new StopPacketGeneratorLearningParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewStopPacketGeneratorLearningParams() *StopPacketGeneratorLearningParams {
	return &StopPacketGeneratorLearningParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewStopPacketGeneratorLearningParamsWithTimeout creates a new StopPacketGeneratorLearningParams object
// with the ability to set a timeout on a request.
func NewStopPacketGeneratorLearningParamsWithTimeout(timeout time.Duration) *StopPacketGeneratorLearningParams {
	return &StopPacketGeneratorLearningParams{
		timeout: timeout,
	}
}

// NewStopPacketGeneratorLearningParamsWithContext creates a new StopPacketGeneratorLearningParams object
// with the ability to set a context for a request.
func NewStopPacketGeneratorLearningParamsWithContext(ctx context.Context) *StopPacketGeneratorLearningParams {
	return &StopPacketGeneratorLearningParams{
		Context: ctx,
	}
}

// NewStopPacketGeneratorLearningParamsWithHTTPClient creates a new StopPacketGeneratorLearningParams object
// with the ability to set a custom HTTPClient for a request.
func NewStopPacketGeneratorLearningParamsWithHTTPClient(client *http.Client) *StopPacketGeneratorLearningParams {
	return &StopPacketGeneratorLearningParams{
		HTTPClient: client,
	}
}

/* StopPacketGeneratorLearningParams contains all the parameters to send to the API endpoint
   for the stop packet generator learning operation.

   Typically these are written to a http.Request.
*/
type StopPacketGeneratorLearningParams struct {

	/* ID.

	   Unique resource identifier

	   Format: string
	*/
	ID string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the stop packet generator learning params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *StopPacketGeneratorLearningParams) WithDefaults() *StopPacketGeneratorLearningParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the stop packet generator learning params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *StopPacketGeneratorLearningParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) WithTimeout(timeout time.Duration) *StopPacketGeneratorLearningParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) WithContext(ctx context.Context) *StopPacketGeneratorLearningParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) WithHTTPClient(client *http.Client) *StopPacketGeneratorLearningParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithID adds the id to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) WithID(id string) *StopPacketGeneratorLearningParams {
	o.SetID(id)
	return o
}

// SetID adds the id to the stop packet generator learning params
func (o *StopPacketGeneratorLearningParams) SetID(id string) {
	o.ID = id
}

// WriteToRequest writes these params to a swagger request
func (o *StopPacketGeneratorLearningParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	// path param id
	if err := r.SetPathParam("id", o.ID); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}