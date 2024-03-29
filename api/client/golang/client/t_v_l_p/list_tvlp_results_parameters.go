// Code generated by go-swagger; DO NOT EDIT.

package t_v_l_p

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

// NewListTvlpResultsParams creates a new ListTvlpResultsParams object,
// with the default timeout for this client.
//
// Default values are not hydrated, since defaults are normally applied by the API server side.
//
// To enforce default values in parameter, use SetDefaults or WithDefaults.
func NewListTvlpResultsParams() *ListTvlpResultsParams {
	return &ListTvlpResultsParams{
		timeout: cr.DefaultTimeout,
	}
}

// NewListTvlpResultsParamsWithTimeout creates a new ListTvlpResultsParams object
// with the ability to set a timeout on a request.
func NewListTvlpResultsParamsWithTimeout(timeout time.Duration) *ListTvlpResultsParams {
	return &ListTvlpResultsParams{
		timeout: timeout,
	}
}

// NewListTvlpResultsParamsWithContext creates a new ListTvlpResultsParams object
// with the ability to set a context for a request.
func NewListTvlpResultsParamsWithContext(ctx context.Context) *ListTvlpResultsParams {
	return &ListTvlpResultsParams{
		Context: ctx,
	}
}

// NewListTvlpResultsParamsWithHTTPClient creates a new ListTvlpResultsParams object
// with the ability to set a custom HTTPClient for a request.
func NewListTvlpResultsParamsWithHTTPClient(client *http.Client) *ListTvlpResultsParams {
	return &ListTvlpResultsParams{
		HTTPClient: client,
	}
}

/* ListTvlpResultsParams contains all the parameters to send to the API endpoint
   for the list tvlp results operation.

   Typically these are written to a http.Request.
*/
type ListTvlpResultsParams struct {
	timeout    time.Duration
	Context    context.Context
	HTTPClient *http.Client
}

// WithDefaults hydrates default values in the list tvlp results params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListTvlpResultsParams) WithDefaults() *ListTvlpResultsParams {
	o.SetDefaults()
	return o
}

// SetDefaults hydrates default values in the list tvlp results params (not the query body).
//
// All values with no default are reset to their zero value.
func (o *ListTvlpResultsParams) SetDefaults() {
	// no default values defined for this parameter
}

// WithTimeout adds the timeout to the list tvlp results params
func (o *ListTvlpResultsParams) WithTimeout(timeout time.Duration) *ListTvlpResultsParams {
	o.SetTimeout(timeout)
	return o
}

// SetTimeout adds the timeout to the list tvlp results params
func (o *ListTvlpResultsParams) SetTimeout(timeout time.Duration) {
	o.timeout = timeout
}

// WithContext adds the context to the list tvlp results params
func (o *ListTvlpResultsParams) WithContext(ctx context.Context) *ListTvlpResultsParams {
	o.SetContext(ctx)
	return o
}

// SetContext adds the context to the list tvlp results params
func (o *ListTvlpResultsParams) SetContext(ctx context.Context) {
	o.Context = ctx
}

// WithHTTPClient adds the HTTPClient to the list tvlp results params
func (o *ListTvlpResultsParams) WithHTTPClient(client *http.Client) *ListTvlpResultsParams {
	o.SetHTTPClient(client)
	return o
}

// SetHTTPClient adds the HTTPClient to the list tvlp results params
func (o *ListTvlpResultsParams) SetHTTPClient(client *http.Client) {
	o.HTTPClient = client
}

// WriteToRequest writes these params to a swagger request
func (o *ListTvlpResultsParams) WriteToRequest(r runtime.ClientRequest, reg strfmt.Registry) error {

	if err := r.SetTimeout(o.timeout); err != nil {
		return err
	}
	var res []error

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}
