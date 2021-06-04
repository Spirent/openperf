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

// TimeCounter A source of clock ticks
//
// swagger:model TimeCounter
type TimeCounter struct {

	// Tick rate of the counter, in Hz.
	// Required: true
	Frequency *int64 `json:"frequency"`

	// Unique time counter identifier
	// Required: true
	ID *string `json:"id"`

	// Time counter name
	// Required: true
	Name *string `json:"name"`

	// Priority determines which counter to use in situations where
	// there are multiple counters available (higher number = higher priority).
	// Priority is always positive.
	//
	// Required: true
	Priority *int32 `json:"priority"`
}

// Validate validates this time counter
func (m *TimeCounter) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateFrequency(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateID(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateName(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validatePriority(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *TimeCounter) validateFrequency(formats strfmt.Registry) error {

	if err := validate.Required("frequency", "body", m.Frequency); err != nil {
		return err
	}

	return nil
}

func (m *TimeCounter) validateID(formats strfmt.Registry) error {

	if err := validate.Required("id", "body", m.ID); err != nil {
		return err
	}

	return nil
}

func (m *TimeCounter) validateName(formats strfmt.Registry) error {

	if err := validate.Required("name", "body", m.Name); err != nil {
		return err
	}

	return nil
}

func (m *TimeCounter) validatePriority(formats strfmt.Registry) error {

	if err := validate.Required("priority", "body", m.Priority); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this time counter based on context it is used
func (m *TimeCounter) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *TimeCounter) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *TimeCounter) UnmarshalBinary(b []byte) error {
	var res TimeCounter
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}