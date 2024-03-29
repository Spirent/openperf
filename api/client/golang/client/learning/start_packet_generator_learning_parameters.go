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

// NewStartPacketGeneratorLearningParams creates a new StartPacketGeneratorLearningParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewStartPacketGeneratorLearningParams() *StartPacketGeneratorLearningParams {
	return &StartPacketGeneratorLearningParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewStartPacketGeneratorLearningParamsWithTimeout creates a new StartPacketGeneratorLearningParams object
// with the ability to set a timeout on a request.
func NewStartPacketGeneratorLearningParamsWithTimeout(timeout time.Duration) *StartPacketGeneratorLearningParams {
	return &StartPacketGeneratorLearningParams{
		timeout: timeout,
	}
}

// NewStartPacketGeneratorLearningParamsWithContext creates a new StartPacketGeneratorLearningParams object
// with the ability to set a context for a request.
func NewStartPacketGeneratorLearningParamsWithContext(ctx context.Context) *StartPacketGeneratorLearningParams {
	return &StartPacketGeneratorLearningParams{
		Context: ctx,
	}
}

// NewStartPacketGeneratorLearningParamsWithHTTPClient creates a new StartPacketGeneratorLearningParams object
// with the ability to set a custom HTTPClient for a request.
func NewStartPacketGeneratorLearningParamsWithHTTPClient(client *http.Client) *StartPacketGeneratorLearningParams {
	return &StartPacketGeneratorLearningParams{
		HTTPClient: client,
	}
}

/* StartPacketGeneratorLearningParams contains all the parameters to send to the API endpoint
   for the start packet generator learning operation.

   Typically these are written to a http.Request.
*/
type StartPacketGeneratorLearningParams struct {

	/* ID.

	   Unique resource identifier

	   Format: string
	*/
	ID string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the start packet generator learning params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *StartPacketGeneratorLearningParams) WithDefaults() *StartPacketGeneratorLearningParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the start packet generator learning params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *StartPacketGeneratorLearningParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) WithTimeout(timeout time.Duration) *StartPacketGeneratorLearningParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) WithContext(ctx context.Context) *StartPacketGeneratorLearningParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) WithHTTPClient(client *http.Client) *StartPacketGeneratorLearningParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithID adds the id to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) WithID(id string) *StartPacketGeneratorLearningParams {
	o.SetID(id)
	return o
}

// SetID adds the id to the start packet generator learning params
func (o *StartPacketGeneratorLearningParams) SetID(id string) {
	o.ID = id
}

// WriteToRequest writes these params to a swagger request
func (o *StartPacketGeneratorLearningParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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
