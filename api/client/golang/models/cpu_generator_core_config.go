// Code generated by go-swagger; DO NOT EDIT.

package models

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"encoding/json"
	"strconv"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"
)

// CPUGeneratorCoreConfig CpuGeneratorCoreConfig
//
// Configuration for a single core
//
// swagger:model CpuGeneratorCoreConfig
type CPUGeneratorCoreConfig struct {

	// Instruction set targets (operations)
	// Required: true
	// Min Items: 1
	Targets []*CPUGeneratorCoreConfigTargetsItems0 `json:"targets"`

	// CPU load generation setpoint
	// Required: true
	// Maximum: 100
	// Minimum: 0
	Utilization *float64 `json:"utilization"`
}

// Validate validates this Cpu generator core config
func (m *CPUGeneratorCoreConfig) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateTargets(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateUtilization(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *CPUGeneratorCoreConfig) validateTargets(formats strfmt.Registry) error {

	if err := validate.Required("targets", "body", m.Targets); err != nil {
		return err
	}

	iTargetsSize := int64(len(m.Targets))

	if err := validate.MinItems("targets", "body", iTargetsSize, 1); err != nil {
		return err
	}

	for i := 0; i < len(m.Targets); i++ {
		if swag.IsZero(m.Targets[i]) { // not required
			continue
		}

		if m.Targets[i] != nil {
			if err := m.Targets[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("targets" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

func (m *CPUGeneratorCoreConfig) validateUtilization(formats strfmt.Registry) error {

	if err := validate.Required("utilization", "body", m.Utilization); err != nil {
		return err
	}

	if err := validate.Minimum("utilization", "body", *m.Utilization, 0, false); err != nil {
		return err
	}

	if err := validate.Maximum("utilization", "body", *m.Utilization, 100, false); err != nil {
		return err
	}

	return nil
}

// ContextValidate validate this Cpu generator core config based on the context it is used
func (m *CPUGeneratorCoreConfig) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := m.contextValidateTargets(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (m *CPUGeneratorCoreConfig) contextValidateTargets(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(m.Targets); i++ {

		if m.Targets[i] != nil {
			if err := m.Targets[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("targets" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (m *CPUGeneratorCoreConfig) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *CPUGeneratorCoreConfig) UnmarshalBinary(b []byte) error {
	var res CPUGeneratorCoreConfig
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}

// CPUGeneratorCoreConfigTargetsItems0 CPU generator core config targets items0
//
// swagger:model CPUGeneratorCoreConfigTargetsItems0
type CPUGeneratorCoreConfigTargetsItems0 struct {

	// CPU load target operation data type, actual for chosen instruction set
	// Required: true
	// Enum: [int32 int64 float32 float64]
	DataType *string `json:"data_type"`

	// CPU load instruction set
	// Required: true
	// Enum: [scalar sse2 sse4 avx avx2 avx512 neon]
	InstructionSet *string `json:"instruction_set"`

	// Targeted load ratio
	// Required: true
	// Minimum: 1
	Weight *int64 `json:"weight"`
}

// Validate validates this CPU generator core config targets items0
func (m *CPUGeneratorCoreConfigTargetsItems0) Validate(formats strfmt.Registry) error {
	var res []error

	if err := m.validateDataType(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateInstructionSet(formats); err != nil {
		res = append(res, err)
	}

	if err := m.validateWeight(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

var cpuGeneratorCoreConfigTargetsItems0TypeDataTypePropEnum []interface{}

func init() {
	var res []string
	if err := json.Unmarshal([]byte(`["int32","int64","float32","float64"]`), &res); err != nil {
		panic(err)
	}
	for _, v := range res {
		cpuGeneratorCoreConfigTargetsItems0TypeDataTypePropEnum = append(cpuGeneratorCoreConfigTargetsItems0TypeDataTypePropEnum, v)
	}
}

const (

	// CPUGeneratorCoreConfigTargetsItems0DataTypeInt32 captures enum value "int32"
	CPUGeneratorCoreConfigTargetsItems0DataTypeInt32 string = "int32"

	// CPUGeneratorCoreConfigTargetsItems0DataTypeInt64 captures enum value "int64"
	CPUGeneratorCoreConfigTargetsItems0DataTypeInt64 string = "int64"

	// CPUGeneratorCoreConfigTargetsItems0DataTypeFloat32 captures enum value "float32"
	CPUGeneratorCoreConfigTargetsItems0DataTypeFloat32 string = "float32"

	// CPUGeneratorCoreConfigTargetsItems0DataTypeFloat64 captures enum value "float64"
	CPUGeneratorCoreConfigTargetsItems0DataTypeFloat64 string = "float64"
)

// prop value enum
func (m *CPUGeneratorCoreConfigTargetsItems0) validateDataTypeEnum(path, location string, value string) error {
	if err := validate.EnumCase(path, location, value, cpuGeneratorCoreConfigTargetsItems0TypeDataTypePropEnum, true); err != nil {
		return err
	}
	return nil
}

func (m *CPUGeneratorCoreConfigTargetsItems0) validateDataType(formats strfmt.Registry) error {

	if err := validate.Required("data_type", "body", m.DataType); err != nil {
		return err
	}

	// value enum
	if err := m.validateDataTypeEnum("data_type", "body", *m.DataType); err != nil {
		return err
	}

	return nil
}

var cpuGeneratorCoreConfigTargetsItems0TypeInstructionSetPropEnum []interface{}

func init() {
	var res []string
	if err := json.Unmarshal([]byte(`["scalar","sse2","sse4","avx","avx2","avx512","neon"]`), &res); err != nil {
		panic(err)
	}
	for _, v := range res {
		cpuGeneratorCoreConfigTargetsItems0TypeInstructionSetPropEnum = append(cpuGeneratorCoreConfigTargetsItems0TypeInstructionSetPropEnum, v)
	}
}

const (

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetScalar captures enum value "scalar"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetScalar string = "scalar"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetSse2 captures enum value "sse2"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetSse2 string = "sse2"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetSse4 captures enum value "sse4"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetSse4 string = "sse4"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx captures enum value "avx"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx string = "avx"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx2 captures enum value "avx2"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx2 string = "avx2"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx512 captures enum value "avx512"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetAvx512 string = "avx512"

	// CPUGeneratorCoreConfigTargetsItems0InstructionSetNeon captures enum value "neon"
	CPUGeneratorCoreConfigTargetsItems0InstructionSetNeon string = "neon"
)

// prop value enum
func (m *CPUGeneratorCoreConfigTargetsItems0) validateInstructionSetEnum(path, location string, value string) error {
	if err := validate.EnumCase(path, location, value, cpuGeneratorCoreConfigTargetsItems0TypeInstructionSetPropEnum, true); err != nil {
		return err
	}
	return nil
}

func (m *CPUGeneratorCoreConfigTargetsItems0) validateInstructionSet(formats strfmt.Registry) error {

	if err := validate.Required("instruction_set", "body", m.InstructionSet); err != nil {
		return err
	}

	// value enum
	if err := m.validateInstructionSetEnum("instruction_set", "body", *m.InstructionSet); err != nil {
		return err
	}

	return nil
}

func (m *CPUGeneratorCoreConfigTargetsItems0) validateWeight(formats strfmt.Registry) error {

	if err := validate.Required("weight", "body", m.Weight); err != nil {
		return err
	}

	if err := validate.MinimumInt("weight", "body", *m.Weight, 1, false); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this CPU generator core config targets items0 based on context it is used
func (m *CPUGeneratorCoreConfigTargetsItems0) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (m *CPUGeneratorCoreConfigTargetsItems0) MarshalBinary() ([]byte, error) {
	if m == nil {
		return nil, nil
	}
	return swag.WriteJSON(m)
}

// UnmarshalBinary interface implementation
func (m *CPUGeneratorCoreConfigTargetsItems0) UnmarshalBinary(b []byte) error {
	var res CPUGeneratorCoreConfigTargetsItems0
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*m = res
	return nil
}
