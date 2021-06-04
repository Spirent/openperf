// Code generated by go-swagger; DO NOT EDIT.

package network_generator

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"github.com/spirent/openperf/api/client/golang/models"
)

// GetNetworkServerReader is a Reader for the GetNetworkServer structure.
type GetNetworkServerReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *GetNetworkServerReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewGetNetworkServerOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetNetworkServerOK creates a GetNetworkServerOK with default headers values
func NewGetNetworkServerOK() *GetNetworkServerOK {
	return &GetNetworkServerOK{}
}

/* GetNetworkServerOK describes a response with status code 200, with default header values.

Success
*/
type GetNetworkServerOK struct {
	Payload *models.NetworkServer
}

func (o *GetNetworkServerOK) Error() string {
	return fmt.Sprintf("[GET /network/servers/{id}][%d] getNetworkServerOK  %+v", 200, o.Payload)
}
func (o *GetNetworkServerOK) GetPayload() *models.NetworkServer {
	return o.Payload
}

func (o *GetNetworkServerOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	o.Payload = new(models.NetworkServer)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}