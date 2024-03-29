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

// TimeKeeper A combination of a time source and a time counter used to measure
// the passage of time, aka a clock
//
//
// swagger:model TimeKeeper
type TimeKeeper struct {

	// The current maximum error in the time calculation, in seconds.
	// Required: true
	MaximumError *float64 `json:"maximum_error"`

	// state
	// Required: true
	State *TimeKeeperState `json:"state"`

	// stats
	// Required: true
	Stats *TimeKeeperStats `json:"stats"`

	// The current time and date in ISO8601 format
	// Required: true
	// Format: date-time
	Time *strfmt.DateTime `json:"time"`

	// time counter used for measuring time intervals
	// Required: true
	TimeCounterID *string `json:"time_counter_id"`

	// time source used for wall-clock synchronization
	// Required: true
	TimeSourceID *string `json:"time_source_id"`
}

// Validate validates this time keeper
func (m *TimeKeeper) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateMaximumError(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateState(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateStats(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTime(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTimeCounterID(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTimeSourceID(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *TimeKeeper) validateMaximumError(formats strfmt.Registry) error {

	if err := validate.Required("maximum_error", "body", m.MaximumError); err != nil {
		return err
	}

	return nil
}

func (m *TimeKeeper) validateState(formats strfmt.Registry) error {

	if err := validate.Required("state", "body", m.State); err != nil {
		return err
	}

	if m.State != nil {
		if err := m.State.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("state")
			}
			return err
		}
	}

	return nil
}

func (m *TimeKeeper) validateStats(formats strfmt.Registry) error {

	if err := validate.Required("stats", "body", m.Stats); err != nil {
		return err
	}

	if m.Stats != nil {
		if err := m.Stats.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("stats")
			}
			return err
		}
	}

	return nil
}

func (m *TimeKeeper) validateTime(formats strfmt.Registry) error {

	if err := validate.Required("time", "body", m.Time); err != nil {
		return err
	}

	if err := validate.FormatOf("time", "body", "date-time", m.Time.String(), formats); err != nil {
		return err
	}

	return nil
}

func (m *TimeKeeper) validateTimeCounterID(formats strfmt.Registry) error {

	if err := validate.Required("time_counter_id", "body", m.TimeCounterID); err != nil {
		return err
	}

	return nil
}

func (m *TimeKeeper) validateTimeSourceID(formats strfmt.Registry) error {

	if err := validate.Required("time_source_id", "body", m.TimeSourceID); err != nil {
		return err
	}

	return nil
}

// ContextValidate validate this time keeper based on the context it is used
func (m *TimeKeeper) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateState(ctx, formats); err != nil {
		res = append(res, err)
	}

	if err := m.contextValidateStats(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *TimeKeeper) contextValidateState(ctx context.Context, formats strfmt.Registry) error {

	if m.State != nil {
		if err := m.State.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("state")
			}
			return err
		}
	}

	return nil
}

func (m *TimeKeeper) contextValidateStats(ctx context.Context, formats strfmt.Registry) error {

	if m.Stats != nil {
		if err := m.Stats.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("stats")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *TimeKeeper) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *TimeKeeper) UnmarshalBinary(b []byte) error {
	var res TimeKeeper
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
