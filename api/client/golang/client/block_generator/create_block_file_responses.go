// Code generated by go-swagger; DO NOT EDIT.

package block_generator

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"fmt"
	"io"

	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"

	"github.com/spirent/openperf/api/client/golang/models"
)

// CreateBlockFileReader is a Reader for the CreateBlockFile structure.
type CreateBlockFileReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *CreateBlockFileReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 201:
		result := NewCreateBlockFileCreated()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewCreateBlockFileCreated creates a CreateBlockFileCreated with default headers values
func NewCreateBlockFileCreated() *CreateBlockFileCreated {
	return &CreateBlockFileCreated{}
}

/* CreateBlockFileCreated describes a response with status code 201, with default header values.

Created
*/
type CreateBlockFileCreated struct {

	/* URI of created block file
	 */
	Location string

	Payload *models.BlockFile
}

func (o *CreateBlockFileCreated) Error() string {
	return fmt.Sprintf("[POST /block-files][%d] createBlockFileCreated  %+v", 201, o.Payload)
}
func (o *CreateBlockFileCreated) GetPayload() *models.BlockFile {
	return o.Payload
}

func (o *CreateBlockFileCreated) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// hydrates response header Location
	hdrLocation := response.GetHeader("Location")

	if hdrLocation != "" {
		o.Location = hdrLocation
	}

	o.Payload = new(models.BlockFile)

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}
