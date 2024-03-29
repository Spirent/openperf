// Code generated by go-swagger; DO NOT EDIT.

package packet_generators

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"github.com/spirent/openperf/api/client/golang/models"
)

// GetPacketGeneratorReader is a Reader for the GetPacketGenerator structure.
type GetPacketGeneratorReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *GetPacketGeneratorReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewGetPacketGeneratorOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetPacketGeneratorOK creates a GetPacketGeneratorOK with default headers values
func NewGetPacketGeneratorOK() *GetPacketGeneratorOK {
	return &GetPacketGeneratorOK{}
}

/* GetPacketGeneratorOK describes a response with status code 200, with default header values.

Success
*/
type GetPacketGeneratorOK struct {
	Payload *models.PacketGenerator
}

func (o *GetPacketGeneratorOK) Error() string {
	return fmt.Sprintf("[GET /packet/generators/{id}][%d] getPacketGeneratorOK  %+v", 200, o.Payload)
}
func (o *GetPacketGeneratorOK) GetPayload() *models.PacketGenerator {
	return o.Payload
}

func (o *GetPacketGeneratorOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.PacketGenerator)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
