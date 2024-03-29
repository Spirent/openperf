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

// PacketGeneratorLearningResultIPV4 Defines an IPv4 address MAC address pair.
//
// swagger:model PacketGeneratorLearningResultIpv4
type PacketGeneratorLearningResultIPV4 struct {

	// IPv4 address.
	// Required: true
	// Pattern: ^((25[0-5]|2[0-4][0-9]|[01]?[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|[01]?[1-9]?[0-9])$
	IPAddress *string `json:"ip_address"`

	// MAC address of associated IPv4 address.
	MacAddress string `json:"mac_address,omitempty"`
}

// Validate validates this packet generator learning result Ipv4
func (m *PacketGeneratorLearningResultIPV4) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateIPAddress(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *PacketGeneratorLearningResultIPV4) validateIPAddress(formats strfmt.Registry) error {

	if err := validate.Required("ip_address", "body", m.IPAddress); err != nil {
		return err
	}

	if err := validate.Pattern("ip_address", "body", *m.IPAddress, `^((25[0-5]|2[0-4][0-9]|[01]?[1-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|[01]?[1-9]?[0-9])$`); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this packet generator learning result Ipv4 based on context it is used
func (m *PacketGeneratorLearningResultIPV4) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *PacketGeneratorLearningResultIPV4) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *PacketGeneratorLearningResultIPV4) UnmarshalBinary(b []byte) error {
	var res PacketGeneratorLearningResultIPV4
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
