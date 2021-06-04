// Code generated by go-swagger; DO NOT EDIT.

package block_generator

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

// NewDeleteBlockFileParams creates a new DeleteBlockFileParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewDeleteBlockFileParams() *DeleteBlockFileParams {
	return &DeleteBlockFileParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewDeleteBlockFileParamsWithTimeout creates a new DeleteBlockFileParams object
// with the ability to set a timeout on a request.
func NewDeleteBlockFileParamsWithTimeout(timeout time.Duration) *DeleteBlockFileParams {
	return &DeleteBlockFileParams{
		timeout: timeout,
	}
}

// NewDeleteBlockFileParamsWithContext creates a new DeleteBlockFileParams object
// with the ability to set a context for a request.
func NewDeleteBlockFileParamsWithContext(ctx context.Context) *DeleteBlockFileParams {
	return &DeleteBlockFileParams{
		Context: ctx,
	}
}

// NewDeleteBlockFileParamsWithHTTPClient creates a new DeleteBlockFileParams object
// with the ability to set a custom HTTPClient for a request.
func NewDeleteBlockFileParamsWithHTTPClient(client *http.Client) *DeleteBlockFileParams {
	return &DeleteBlockFileParams{
		HTTPClient: client,
	}
}

/* DeleteBlockFileParams contains all the parameters to send to the API endpoint
   for the delete block file operation.

   Typically these are written to a http.Request.
*/
type DeleteBlockFileParams struct {

	/* ID.

	   Unique resource identifier

	   Format: string
	*/
	ID string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the delete block file params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeleteBlockFileParams) WithDefaults() *DeleteBlockFileParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the delete block file params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *DeleteBlockFileParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the delete block file params
func (o *DeleteBlockFileParams) WithTimeout(timeout time.Duration) *DeleteBlockFileParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the delete block file params
func (o *DeleteBlockFileParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the delete block file params
func (o *DeleteBlockFileParams) WithContext(ctx context.Context) *DeleteBlockFileParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the delete block file params
func (o *DeleteBlockFileParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the delete block file params
func (o *DeleteBlockFileParams) WithHTTPClient(client *http.Client) *DeleteBlockFileParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the delete block file params
func (o *DeleteBlockFileParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithID adds the id to the delete block file params
func (o *DeleteBlockFileParams) WithID(id string) *DeleteBlockFileParams {
	o.SetID(id)
	return o
}

// SetID adds the id to the delete block file params
func (o *DeleteBlockFileParams) SetID(id string) {
	o.ID = id
}

// WriteToRequest writes these params to a swagger request
func (o *DeleteBlockFileParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

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