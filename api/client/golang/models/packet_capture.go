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

// PacketCapture Packet capture; captures packets.
//
//
// swagger:model PacketCapture
type PacketCapture struct {

	// Indicates whether this object is currently capturing packets or not.
	//
	// Required: true
	Active *bool `json:"active"`

	// config
	// Required: true
	Config *PacketCaptureConfig `json:"config"`

	// Packet capture direction
	// Required: true
	// Enum: [rx tx rx_and_tx]
	Direction *string `json:"direction"`

	// Unique capture identifier
	// Required: true
	ID *string `json:"id"`

	// Specifies the unique source of packets for this capture. This
	// id may refer to either a port or an interface.
	//
	// Required: true
	SourceID *string `json:"source_id"`
}

// Validate validates this packet capture
func (m *PacketCapture) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateActive(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateConfig(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateDirection(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateID(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateSourceID(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *PacketCapture) validateActive(formats strfmt.Registry) error {

	if err := validate.Required("active", "body", m.Active); err != nil {
		return err
	}

	return nil
}

func (m *PacketCapture) validateConfig(formats strfmt.Registry) error {

	if err := validate.Required("config", "body", m.Config); err != nil {
		return err
	}

	if m.Config != nil {
		if err := m.Config.Validate(formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("config")
			}
			return err
		}
	}

	return nil
}

var packetCaptureTypeDirectionPropEnum []interface{}

func init() {
	var res []string
	if err := json.Unmarshal([]byte(`["rx","tx","rx_and_tx"]`), &res); err != nil {
		panic(err)
	}
	for _, v := range res {
		packetCaptureTypeDirectionPropEnum = append(packetCaptureTypeDirectionPropEnum, v)
	}
}

const (

	// PacketCaptureDirectionRx captures enum value "rx"
	PacketCaptureDirectionRx string = "rx"

	// PacketCaptureDirectionTx captures enum value "tx"
	PacketCaptureDirectionTx string = "tx"

	// PacketCaptureDirectionRxAndTx captures enum value "rx_and_tx"
	PacketCaptureDirectionRxAndTx string = "rx_and_tx"
)

// prop value enum
func (m *PacketCapture) validateDirectionEnum(path, location string, value string) error {
	if err := validate.EnumCase(path, location, value, packetCaptureTypeDirectionPropEnum, true); err != nil {
		return err
	}
	return nil
}

func (m *PacketCapture) validateDirection(formats strfmt.Registry) error {

	if err := validate.Required("direction", "body", m.Direction); err != nil {
		return err
	}

	// value enum
	if err := m.validateDirectionEnum("direction", "body", *m.Direction); err != nil {
		return err
	}

	return nil
}

func (m *PacketCapture) validateID(formats strfmt.Registry) error {

	if err := validate.Required("id", "body", m.ID); err != nil {
		return err
	}

	return nil
}

func (m *PacketCapture) validateSourceID(formats strfmt.Registry) error {

	if err := validate.Required("source_id", "body", m.SourceID); err != nil {
		return err
	}

	return nil
}

// ContextValidate validate this packet capture based on the context it is used
func (m *PacketCapture) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateConfig(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *PacketCapture) contextValidateConfig(ctx context.Context, formats strfmt.Registry) error {

	if m.Config != nil {
		if err := m.Config.ContextValidate(ctx, formats); err != nil {
			if ve, ok := err.(*errors.Validation); ok {
				return ve.ValidateName("config")
			}
			return err
		}
	}

	return nil
}

// MarshalBinary interface implementation
func (m *PacketCapture) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *PacketCapture) UnmarshalBinary(b []byte) error {
	var res PacketCapture
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
