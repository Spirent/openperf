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

// MemoryGeneratorResult Results collected by a running generator
//
// swagger:model MemoryGeneratorResult
type MemoryGeneratorResult struct {

	// Indicates whether the result is currently being updated
	// Required: true
	Active *bool `json:"active"`

	// dynamic results
	DynamicResults *DynamicResults `json:"dynamic_results,omitempty"`

	// Memory generator identifier that generated this result
	GeneratorID string `json:"generator_id,omitempty"`

	// Unique generator result identifier
	// Required: true
	ID *string `json:"id"`

	// read
	// Required: true
	Read *MemoryGeneratorStats `json:"read"`

	// The ISO8601-formatted start time of the generator
	// Required: true
	// Format: date-time
	TimestampFirst *strfmt.DateTime `json:"timestamp_first"`

	// The ISO8601-formatted date of the last result update
	// Required: true
	// Format: date-time
	TimestampLast *strfmt.DateTime `json:"timestamp_last"`

	// write
	// Required: true
	Write *MemoryGeneratorStats `json:"write"`
}

// Validate validates this memory generator result
func (m *MemoryGeneratorResult) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateActive(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateDynamicResults(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateID(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateRead(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTimestampFirst(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTimestampLast(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateWrite(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *MemoryGeneratorResult) validateActive(formats strfmt.Registry) error {

	if err := validate.Required("active", "body", m.Active); err != nil {
		return err
	}

	return nil
}

func (m *MemoryGeneratorResult) validateDynamicResults(formats strfmt.Registry) error {
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

func (m *MemoryGeneratorResult) validateID(formats strfmt.Registry) error {

	if err := validate.Required("id", "body", m.ID); err != nil {
		return err
	}

	return nil
}

func (m *MemoryGeneratorResult) validateRead(formats strfmt.Registry) error {

	if err := validate.Required("read", "body", m.Read); err != nil {
		return err
	}

	if m.Read != nil {
		if err := m.Read.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("read")
			}
			return err
		}
	}

	return nil
}

func (m *MemoryGeneratorResult) validateTimestampFirst(formats strfmt.Registry) error {

	if err := validate.Required("timestamp_first", "body", m.TimestampFirst); err != nil {
		return err
	}

	if err := validate.FormatOf("timestamp_first", "body", "date-time", m.TimestampFirst.String(), formats); err != nil {
		return err
	}

	return nil
}

func (m *MemoryGeneratorResult) validateTimestampLast(formats strfmt.Registry) error {

	if err := validate.Required("timestamp_last", "body", m.TimestampLast); err != nil {
		return err
	}

	if err := validate.FormatOf("timestamp_last", "body", "date-time", m.TimestampLast.String(), formats); err != nil {
		return err
	}

	return nil
}

func (m *MemoryGeneratorResult) validateWrite(formats strfmt.Registry) error {

	if err := validate.Required("write", "body", m.Write); err != nil {
		return err
	}

	if m.Write != nil {
		if err := m.Write.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("write")
			}
			return err
		}
	}

	return nil
}

// ContextValidate validate this memory generator result based on the context it is used
func (m *MemoryGeneratorResult) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateDynamicResults(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateRead(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateWrite(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *MemoryGeneratorResult) contextValidateDynamicResults(ctx context.Context, formats strfmt.Registry) error {

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

func (m *MemoryGeneratorResult) contextValidateRead(ctx context.Context, formats strfmt.Registry) error {

	if m.Read != nil {
		if err := m.Read.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("read")
			}
			return err
		}
	}

	return nil
}

func (m *MemoryGeneratorResult) contextValidateWrite(ctx context.Context, formats strfmt.Registry) error {

	if m.Write != nil {
		if err := m.Write.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("write")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *MemoryGeneratorResult) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *MemoryGeneratorResult) UnmarshalBinary(b []byte) error {
	var res MemoryGeneratorResult
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
