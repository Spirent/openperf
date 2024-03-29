// Code generated by go-swagger; DO NOT EDIT.

package cpu_generator

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

// NewBulkCreateCPUGeneratorsParams creates a new BulkCreateCPUGeneratorsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewBulkCreateCPUGeneratorsParams() *BulkCreateCPUGeneratorsParams {
	return &BulkCreateCPUGeneratorsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewBulkCreateCPUGeneratorsParamsWithTimeout creates a new BulkCreateCPUGeneratorsParams object
// with the ability to set a timeout on a request.
func NewBulkCreateCPUGeneratorsParamsWithTimeout(timeout time.Duration) *BulkCreateCPUGeneratorsParams {
	return &BulkCreateCPUGeneratorsParams{
		timeout: timeout,
	}
}

// NewBulkCreateCPUGeneratorsParamsWithContext creates a new BulkCreateCPUGeneratorsParams object
// with the ability to set a context for a request.
func NewBulkCreateCPUGeneratorsParamsWithContext(ctx context.Context) *BulkCreateCPUGeneratorsParams {
	return &BulkCreateCPUGeneratorsParams{
		Context: ctx,
	}
}

// NewBulkCreateCPUGeneratorsParamsWithHTTPClient creates a new BulkCreateCPUGeneratorsParams object
// with the ability to set a custom HTTPClient for a request.
func NewBulkCreateCPUGeneratorsParamsWithHTTPClient(client *http.Client) *BulkCreateCPUGeneratorsParams {
	return &BulkCreateCPUGeneratorsParams{
		HTTPClient: client,
	}
}

/* BulkCreateCPUGeneratorsParams contains all the parameters to send to the API endpoint
   for the bulk create Cpu generators operation.

   Typically these are written to a http.Request.
*/
type BulkCreateCPUGeneratorsParams struct {

	/* Create.

	   Bulk creation
	*/
	Create BulkCreateCPUGeneratorsBody

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the bulk create Cpu generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *BulkCreateCPUGeneratorsParams) WithDefaults() *BulkCreateCPUGeneratorsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the bulk create Cpu generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *BulkCreateCPUGeneratorsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) WithTimeout(timeout time.Duration) *BulkCreateCPUGeneratorsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) WithContext(ctx context.Context) *BulkCreateCPUGeneratorsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) WithHTTPClient(client *http.Client) *BulkCreateCPUGeneratorsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithCreate adds the create to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) WithCreate(create BulkCreateCPUGeneratorsBody) *BulkCreateCPUGeneratorsParams {
	o.SetCreate(create)
	return o
}

// SetCreate adds the create to the bulk create Cpu generators params
func (o *BulkCreateCPUGeneratorsParams) SetCreate(create BulkCreateCPUGeneratorsBody) {
	o.Create = create
}

// WriteToRequest writes these params to a swagger request
func (o *BulkCreateCPUGeneratorsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error
	if err := r.SetBodyParam(o.Create); err != nil {
		return err
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
