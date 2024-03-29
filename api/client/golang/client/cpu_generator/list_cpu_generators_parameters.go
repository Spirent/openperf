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

// NewListCPUGeneratorsParams creates a new ListCPUGeneratorsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewListCPUGeneratorsParams() *ListCPUGeneratorsParams {
	return &ListCPUGeneratorsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewListCPUGeneratorsParamsWithTimeout creates a new ListCPUGeneratorsParams object
// with the ability to set a timeout on a request.
func NewListCPUGeneratorsParamsWithTimeout(timeout time.Duration) *ListCPUGeneratorsParams {
	return &ListCPUGeneratorsParams{
		timeout: timeout,
	}
}

// NewListCPUGeneratorsParamsWithContext creates a new ListCPUGeneratorsParams object
// with the ability to set a context for a request.
func NewListCPUGeneratorsParamsWithContext(ctx context.Context) *ListCPUGeneratorsParams {
	return &ListCPUGeneratorsParams{
		Context: ctx,
	}
}

// NewListCPUGeneratorsParamsWithHTTPClient creates a new ListCPUGeneratorsParams object
// with the ability to set a custom HTTPClient for a request.
func NewListCPUGeneratorsParamsWithHTTPClient(client *http.Client) *ListCPUGeneratorsParams {
	return &ListCPUGeneratorsParams{
		HTTPClient: client,
	}
}

/* ListCPUGeneratorsParams contains all the parameters to send to the API endpoint
   for the list Cpu generators operation.

   Typically these are written to a http.Request.
*/
type ListCPUGeneratorsParams struct {
	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the list Cpu generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListCPUGeneratorsParams) WithDefaults() *ListCPUGeneratorsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the list Cpu generators params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListCPUGeneratorsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the list Cpu generators params
func (o *ListCPUGeneratorsParams) WithTimeout(timeout time.Duration) *ListCPUGeneratorsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the list Cpu generators params
func (o *ListCPUGeneratorsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the list Cpu generators params
func (o *ListCPUGeneratorsParams) WithContext(ctx context.Context) *ListCPUGeneratorsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the list Cpu generators params
func (o *ListCPUGeneratorsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the list Cpu generators params
func (o *ListCPUGeneratorsParams) WithHTTPClient(client *http.Client) *ListCPUGeneratorsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the list Cpu generators params
func (o *ListCPUGeneratorsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WriteToRequest writes these params to a swagger request
func (o *ListCPUGeneratorsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
