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

// StackProtocolStats Stack per-protocol statistics
//
// swagger:model StackProtocolStats
type StackProtocolStats struct {

	// Cache hits
	// Required: true
	CacheHits *int64 `json:"cache_hits"`

	// Checksum errors
	// Required: true
	ChecksumErrors *int64 `json:"checksum_errors"`

	// Dropped packets
	// Required: true
	DroppedPackets *int64 `json:"dropped_packets"`

	// Forwarded packets
	// Required: true
	ForwardedPackets *int64 `json:"forwarded_packets"`

	// Invalid length errors
	// Required: true
	LengthErrors *int64 `json:"length_errors"`

	// Out of memory errors
	// Required: true
	MemoryErrors *int64 `json:"memory_errors"`

	// Miscellaneous errors
	// Required: true
	MiscErrors *int64 `json:"misc_errors"`

	// Errors in options
	// Required: true
	OptionErrors *int64 `json:"option_errors"`

	// Protocol error
	// Required: true
	ProtocolErrors *int64 `json:"protocol_errors"`

	// Routing errors
	// Required: true
	RoutingErrors *int64 `json:"routing_errors"`

	// Received packets
	// Required: true
	RxPackets *int64 `json:"rx_packets"`

	// Transmitted packets
	// Required: true
	TxPackets *int64 `json:"tx_packets"`
}

// Validate validates this stack protocol stats
func (m *StackProtocolStats) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateCacheHits(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateChecksumErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateDroppedPackets(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateForwardedPackets(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateLengthErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateMemoryErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateMiscErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateOptionErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateProtocolErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateRoutingErrors(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateRxPackets(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateTxPackets(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *StackProtocolStats) validateCacheHits(formats strfmt.Registry) error {

	if err := validate.Required("cache_hits", "body", m.CacheHits); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateChecksumErrors(formats strfmt.Registry) error {

	if err := validate.Required("checksum_errors", "body", m.ChecksumErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateDroppedPackets(formats strfmt.Registry) error {

	if err := validate.Required("dropped_packets", "body", m.DroppedPackets); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateForwardedPackets(formats strfmt.Registry) error {

	if err := validate.Required("forwarded_packets", "body", m.ForwardedPackets); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateLengthErrors(formats strfmt.Registry) error {

	if err := validate.Required("length_errors", "body", m.LengthErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateMemoryErrors(formats strfmt.Registry) error {

	if err := validate.Required("memory_errors", "body", m.MemoryErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateMiscErrors(formats strfmt.Registry) error {

	if err := validate.Required("misc_errors", "body", m.MiscErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateOptionErrors(formats strfmt.Registry) error {

	if err := validate.Required("option_errors", "body", m.OptionErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateProtocolErrors(formats strfmt.Registry) error {

	if err := validate.Required("protocol_errors", "body", m.ProtocolErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateRoutingErrors(formats strfmt.Registry) error {

	if err := validate.Required("routing_errors", "body", m.RoutingErrors); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateRxPackets(formats strfmt.Registry) error {

	if err := validate.Required("rx_packets", "body", m.RxPackets); err != nil {
		return err
	}

	return nil
}

func (m *StackProtocolStats) validateTxPackets(formats strfmt.Registry) error {

	if err := validate.Required("tx_packets", "body", m.TxPackets); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this stack protocol stats based on context it is used
func (m *StackProtocolStats) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *StackProtocolStats) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *StackProtocolStats) UnmarshalBinary(b []byte) error {
	var res StackProtocolStats
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}