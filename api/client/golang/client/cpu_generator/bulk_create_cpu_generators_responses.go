// Code generated by go-swagger; DO NOT EDIT.

package cpu_generator

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"fmt"
	"io"
	"strconv"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"

	"github.com/spirent/openperf/api/client/golang/models"
)

// BulkCreateCPUGeneratorsReader is a Reader for the BulkCreateCPUGenerators structure.
type BulkCreateCPUGeneratorsReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *BulkCreateCPUGeneratorsReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewBulkCreateCPUGeneratorsOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewBulkCreateCPUGeneratorsOK creates a BulkCreateCPUGeneratorsOK with default headers values
func NewBulkCreateCPUGeneratorsOK() *BulkCreateCPUGeneratorsOK {
	return &BulkCreateCPUGeneratorsOK{}
}

/* BulkCreateCPUGeneratorsOK describes a response with status code 200, with default header values.

Success
*/
type BulkCreateCPUGeneratorsOK struct {
	Payload []*models.CPUGenerator
}

func (o *BulkCreateCPUGeneratorsOK) Error() string {
	return fmt.Sprintf("[POST /cpu-generators/x/bulk-create][%d] bulkCreateCpuGeneratorsOK  %+v", 200, o.Payload)
}
func (o *BulkCreateCPUGeneratorsOK) GetPayload() []*models.CPUGenerator {
	return o.Payload
}

func (o *BulkCreateCPUGeneratorsOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// response payload
	if err := consumer.Consume(response.Body(), &o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

/*BulkCreateCPUGeneratorsBody BulkCreateCpuGeneratorsRequest
//
// Parameters for the bulk create operation
swagger:model BulkCreateCPUGeneratorsBody
*/
type BulkCreateCPUGeneratorsBody struct {

	// List of CPU generators
	// Required: true
	// Min Items: 1
	Items []*models.CPUGenerator `json:"items"`
}

// Validate validates this bulk create CPU generators body
func (o *BulkCreateCPUGeneratorsBody) Validate(formats strfmt.Registry) error {
	var res []error

	if err := o.validateItems(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (o *BulkCreateCPUGeneratorsBody) validateItems(formats strfmt.Registry) error {

	if err := validate.Required("create"+"."+"items", "body", o.Items); err != nil {
		return err
	}

	iItemsSize := int64(len(o.Items))

	if err := validate.MinItems("create"+"."+"items", "body", iItemsSize, 1); err != nil {
		return err
	}

	for i := 0; i < len(o.Items); i++ {
		if swag.IsZero(o.Items[i]) { // not required
			continue
		}

		if o.Items[i] != nil {
			if err := o.Items[i].Validate(formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("create" + "." + "items" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// ContextValidate validate this bulk create CPU generators body based on the context it is used
func (o *BulkCreateCPUGeneratorsBody) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	var res []error

	if err := o.contextValidateItems(ctx, formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (o *BulkCreateCPUGeneratorsBody) contextValidateItems(ctx context.Context, formats strfmt.Registry) error {

	for i := 0; i < len(o.Items); i++ {

		if o.Items[i] != nil {
			if err := o.Items[i].ContextValidate(ctx, formats); err != nil {
				if ve, ok := err.(*errors.Validation); ok {
					return ve.ValidateName("create" + "." + "items" + "." + strconv.Itoa(i))
				}
				return err
			}
		}

	}

	return nil
}

// MarshalBinary interface implementation
func (o *BulkCreateCPUGeneratorsBody) MarshalBinary() ([]byte, error) {
	if o == nil {
		return nil, nil
	}
	return swag.WriteJSON(o)
}

// UnmarshalBinary interface implementation
func (o *BulkCreateCPUGeneratorsBody) UnmarshalBinary(b []byte) error {
	var res BulkCreateCPUGeneratorsBody
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*o = res
	return nil
}
