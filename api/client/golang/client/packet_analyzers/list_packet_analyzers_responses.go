// Code generated by go-swagger; DO NOT EDIT.

package packet_analyzers

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"github.com/spirent/openperf/api/client/golang/models"
)

// ListPacketAnalyzersReader is a Reader for the ListPacketAnalyzers structure.
type ListPacketAnalyzersReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *ListPacketAnalyzersReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewListPacketAnalyzersOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewListPacketAnalyzersOK creates a ListPacketAnalyzersOK with default headers values
func NewListPacketAnalyzersOK() *ListPacketAnalyzersOK {
	return &ListPacketAnalyzersOK{}
}

/* ListPacketAnalyzersOK describes a response with status code 200, with default header values.

Success
*/
type ListPacketAnalyzersOK struct {
	Payload []*models.PacketAnalyzer
}

func (o *ListPacketAnalyzersOK) Error() string {
	return fmt.Sprintf("[GET /packet/analyzers][%d] listPacketAnalyzersOK  %+v", 200, o.Payload)
}
func (o *ListPacketAnalyzersOK) GetPayload() []*models.PacketAnalyzer {
	return o.Payload
}

func (o *ListPacketAnalyzersOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// response payload
	if err := consumer.Consume(response.Body(), &o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
