// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"
)

// ToggleNetworkGeneratorsRequest Parameters for the toggle operation
//
// swagger:model ToggleNetworkGeneratorsRequest
type ToggleNetworkGeneratorsRequest struct {

	// dynamic results
	DynamicResults *DynamicResultsConfig `json:"dynamic_results,omitempty"`

	// The unique id of the running network generator
	// Required: true
	Replace *string `json:"replace"`

	// The unique id of the stopped network generator
	// Required: true
	With *string `json:"with"`
}

// Validate validates this toggle network generators request
func (m *ToggleNetworkGeneratorsRequest) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateDynamicResults(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateReplace(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateWith(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *ToggleNetworkGeneratorsRequest) validateDynamicResults(formats strfmt.Registry) error {
	if swag.IsZero(m.DynamicResults) { // not required
		return nil
	}

	if m.DynamicResults != nil {
		if err := m.DynamicResults.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("dynamic_results")
			}
			return err
		}
	}

	return nil
}

func (m *ToggleNetworkGeneratorsRequest) validateReplace(formats strfmt.Registry) error {

	if err := validate.Required("replace", "body", m.Replace); err != nil {
		return err
	}

	return nil
}

func (m *ToggleNetworkGeneratorsRequest) validateWith(formats strfmt.Registry) error {

	if err := validate.Required("with", "body", m.With); err != nil {
		return err
	}

	return nil
}

// ContextValidate validate this toggle network generators request based on the context it is used
func (m *ToggleNetworkGeneratorsRequest) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateDynamicResults(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *ToggleNetworkGeneratorsRequest) contextValidateDynamicResults(ctx context.Context, formats strfmt.Registry) error {

	if m.DynamicResults != nil {
		if err := m.DynamicResults.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("dynamic_results")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *ToggleNetworkGeneratorsRequest) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *ToggleNetworkGeneratorsRequest) UnmarshalBinary(b []byte) error {
	var res ToggleNetworkGeneratorsRequest
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}