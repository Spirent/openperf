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

// ListTxFlowsReader is a Reader for the ListTxFlows structure.
type ListTxFlowsReader struct {
	formats strfmt.Registry
}

// ReadResponse reads a server response into the received o.
func (o *ListTxFlowsReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewListTxFlowsOK()
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewListTxFlowsOK creates a ListTxFlowsOK with default headers values
func NewListTxFlowsOK() *ListTxFlowsOK {
	return &ListTxFlowsOK{}
}

/* ListTxFlowsOK describes a response with status code 200, with default header values.

Success
*/
type ListTxFlowsOK struct {
	Payload []*models.TxFlow
}

func (o *ListTxFlowsOK) Error() string {
	return fmt.Sprintf("[GET /packet/tx-flows][%d] listTxFlowsOK  %+v", 200, o.Payload)
}
func (o *ListTxFlowsOK) GetPayload() []*models.TxFlow {
	return o.Payload
}

func (o *ListTxFlowsOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// response payload
	if err := consumer.Consume(response.Body(), &o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}