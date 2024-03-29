// Code generated by go-swagger; DO NOT EDIT.

package ports

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

// NewListPortsParams creates a new ListPortsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewListPortsParams() *ListPortsParams {
	return &ListPortsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewListPortsParamsWithTimeout creates a new ListPortsParams object
// with the ability to set a timeout on a request.
func NewListPortsParamsWithTimeout(timeout time.Duration) *ListPortsParams {
	return &ListPortsParams{
		timeout: timeout,
	}
}

// NewListPortsParamsWithContext creates a new ListPortsParams object
// with the ability to set a context for a request.
func NewListPortsParamsWithContext(ctx context.Context) *ListPortsParams {
	return &ListPortsParams{
		Context: ctx,
	}
}

// NewListPortsParamsWithHTTPClient creates a new ListPortsParams object
// with the ability to set a custom HTTPClient for a request.
func NewListPortsParamsWithHTTPClient(client *http.Client) *ListPortsParams {
	return &ListPortsParams{
		HTTPClient: client,
	}
}

/* ListPortsParams contains all the parameters to send to the API endpoint
   for the list ports operation.

   Typically these are written to a http.Request.
*/
type ListPortsParams struct {

	/* Kind.

	   Filter by kind
	*/
	Kind *string

	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the list ports params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListPortsParams) WithDefaults() *ListPortsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the list ports params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListPortsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the list ports params
func (o *ListPortsParams) WithTimeout(timeout time.Duration) *ListPortsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the list ports params
func (o *ListPortsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the list ports params
func (o *ListPortsParams) WithContext(ctx context.Context) *ListPortsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the list ports params
func (o *ListPortsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the list ports params
func (o *ListPortsParams) WithHTTPClient(client *http.Client) *ListPortsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the list ports params
func (o *ListPortsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WithKind adds the kind to the list ports params
func (o *ListPortsParams) WithKind(kind *string) *ListPortsParams {
	o.SetKind(kind)
	return o
}

// SetKind adds the kind to the list ports params
func (o *ListPortsParams) SetKind(kind *string) {
	o.Kind = kind
}

// WriteToRequest writes these params to a swagger request
func (o *ListPortsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if o.Kind != nil {

		// query param kind
		var qrKind string

		if o.Kind != nil {
			qrKind = *o.Kind
		}
		qKind := qrKind
		if qKind != "" {

			if err := r.SetQueryParam("kind", qKind); err != nil {
				return err
			}
		}
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
