// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"encoding/json"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"
)

// TrafficDuration Describes how long a packet generator should transmit traffic once
// started. Only one property may be set.
//
//
// swagger:model TrafficDuration
type TrafficDuration struct {

	// Indicates there is no duration limit when set.
	Continuous bool `json:"continuous,omitempty"`

	// Specifies the duration as number of transmitted frames.
	// Minimum: 1
	Frames int32 `json:"frames,omitempty"`

	// time
	Time *TrafficDurationTime `json:"time,omitempty"`
}

// Validate validates this traffic duration
func (m *TrafficDuration) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateFrames(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTime(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *TrafficDuration) validateFrames(formats strfmt.Registry) error {
	if swag.IsZero(m.Frames) { // not required
		return nil
	}

	if err := validate.MinimumInt("frames", "body", int64(m.Frames), 1, false); err != nil {
		return err
	}

	return nil
}

func (m *TrafficDuration) validateTime(formats strfmt.Registry) error {
	if swag.IsZero(m.Time) { // not required
		return nil
	}

	if m.Time != nil {
		if err := m.Time.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("time")
			}
			return err
		}
	}

	return nil
}

// ContextValidate validate this traffic duration based on the context it is used
func (m *TrafficDuration) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateTime(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *TrafficDuration) contextValidateTime(ctx context.Context, formats strfmt.Registry) error {

	if m.Time != nil {
		if err := m.Time.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("time")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *TrafficDuration) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *TrafficDuration) UnmarshalBinary(b []byte) error {
	var res TrafficDuration
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}

// TrafficDurationTime Describes the transmit time
//
// swagger:model TrafficDurationTime
type TrafficDurationTime struct {

	// Specifies the units for value
	// Required: true
	// Enum: [hours minutes seconds milliseconds]
	Units *string `json:"units"`

	// Specifies the value for time based transmission
	// Required: true
	// Minimum: 1
	Value *int32 `json:"value"`
}

// Validate validates this traffic duration time
func (m *TrafficDurationTime) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateUnits(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateValue(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

var trafficDurationTimeTypeUnitsPropEnum []interface{}

func init() {
	var res []string
	if err := json.Unmarshal([]byte(`["hours","minutes","seconds","milliseconds"]`), &res); err != nil {
		panic(err)
	}
	for _, v := range res {
		trafficDurationTimeTypeUnitsPropEnum = append(trafficDurationTimeTypeUnitsPropEnum, v)
	}
}

const (

	// TrafficDurationTimeUnitsHours captures enum value "hours"
	TrafficDurationTimeUnitsHours string = "hours"

	// TrafficDurationTimeUnitsMinutes captures enum value "minutes"
	TrafficDurationTimeUnitsMinutes string = "minutes"

	// TrafficDurationTimeUnitsSeconds captures enum value "seconds"
	TrafficDurationTimeUnitsSeconds string = "seconds"

	// TrafficDurationTimeUnitsMilliseconds captures enum value "milliseconds"
	TrafficDurationTimeUnitsMilliseconds string = "milliseconds"
)

// prop value enum
func (m *TrafficDurationTime) validateUnitsEnum(path, location string, value string) error {
	if err := validate.EnumCase(path, location, value, trafficDurationTimeTypeUnitsPropEnum, true); err != nil {
		return err
	}
	return nil
}

func (m *TrafficDurationTime) validateUnits(formats strfmt.Registry) error {

	if err := validate.Required("time"+"."+"units", "body", m.Units); err != nil {
		return err
	}

	// value enum
	if err := m.validateUnitsEnum("time"+"."+"units", "body", *m.Units); err != nil {
		return err
	}

	return nil
}

func (m *TrafficDurationTime) validateValue(formats strfmt.Registry) error {

	if err := validate.Required("time"+"."+"value", "body", m.Value); err != nil {
		return err
	}

	if err := validate.MinimumInt("time"+"."+"value", "body", int64(*m.Value), 1, false); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this traffic duration time based on context it is used
func (m *TrafficDurationTime) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *TrafficDurationTime) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *TrafficDurationTime) UnmarshalBinary(b []byte) error {
	var res TrafficDurationTime
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
