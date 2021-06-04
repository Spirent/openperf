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

// CreateNetworkServerReader is a Reader for the CreateNetworkServer structure.
type CreateNetworkServerReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *CreateNetworkServerReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 201:
		result := NewCreateNetworkServerCreated()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewCreateNetworkServerCreated creates a CreateNetworkServerCreated with default headers values
func NewCreateNetworkServerCreated() *CreateNetworkServerCreated {
	return &CreateNetworkServerCreated{}
}

/* CreateNetworkServerCreated describes a response with status code 201, with default header values.

Created
*/
type CreateNetworkServerCreated struct {

	/* URI of created network server
	 */
	Location string

	Payload *models.NetworkServer
}

func (o *CreateNetworkServerCreated) Error() string {
	return fmt.Sprintf("[POST /network/servers][%d] createNetworkServerCreated  %+v", 201, o.Payload)
}
func (o *CreateNetworkServerCreated) GetPayload() *models.NetworkServer {
	return o.Payload
}

func (o *CreateNetworkServerCreated) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// hydrates response header Location
	hdrLocation := response.GetHeader("Location")

	if hdrLocation != "" {
		o.Location = hdrLocation
	}

	o.Payload = new(models.NetworkServer)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}