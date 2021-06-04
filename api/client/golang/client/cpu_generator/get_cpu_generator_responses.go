// Code generated by go-swagger; DO NOT EDIT.

package cpu_generator

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"github.com/spirent/openperf/api/client/golang/models"
)

// GetCPUGeneratorReader is a Reader for the GetCPUGenerator structure.
type GetCPUGeneratorReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *GetCPUGeneratorReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewGetCPUGeneratorOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetCPUGeneratorOK creates a GetCPUGeneratorOK with default headers values
func NewGetCPUGeneratorOK() *GetCPUGeneratorOK {
	return &GetCPUGeneratorOK{}
}

/* GetCPUGeneratorOK describes a response with status code 200, with default header values.

Success
*/
type GetCPUGeneratorOK struct {
	Payload *models.CPUGenerator
}

func (o *GetCPUGeneratorOK) Error() string {
	return fmt.Sprintf("[GET /cpu-generators/{id}][%d] getCpuGeneratorOK  %+v", 200, o.Payload)
}
func (o *GetCPUGeneratorOK) GetPayload() *models.CPUGenerator {
	return o.Payload
}

func (o *GetCPUGeneratorOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.CPUGenerator)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}