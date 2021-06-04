// Code generated by go-swagger; DO NOT EDIT.

package packet_captures

// This file was generated by the swagger tool.
// Editing this file might prove futile when you re-run the swagger generate command

import (
	"context"
	"fmt"
	"io"

	"github.com/go-openapi/errors"
	"github.com/go-openapi/runtime"
	"github.com/go-openapi/strfmt"
	"github.com/go-openapi/swag"
	"github.com/go-openapi/validate"
)

// GetPacketCapturesPcapReader is a Reader for the GetPacketCapturesPcap structure.
type GetPacketCapturesPcapReader struct {
	formats strfmt.Registry
	writer  io.Writer
}

// ReadResponse reads a server response into the received o.
func (o *GetPacketCapturesPcapReader) ReadResponse(response runtime.ClientResponse, consumer runtime.Consumer) (interface{}, error) {
	switch response.Code() {
	case 200:
		result := NewGetPacketCapturesPcapOK(o.writer)
		if err := result.readResponse(response, consumer, o.formats); err != nil {
			return nil, err
		}
		return result, nil
	default:
		return nil, runtime.NewAPIError("response status code does not match any response statuses defined for this endpoint in the swagger spec", response, response.Code())
	}
}

// NewGetPacketCapturesPcapOK creates a GetPacketCapturesPcapOK with default headers values
func NewGetPacketCapturesPcapOK(writer io.Writer) *GetPacketCapturesPcapOK {
	return &GetPacketCapturesPcapOK{

		Payload: writer,
	}
}

/* GetPacketCapturesPcapOK describes a response with status code 200, with default header values.

Success
*/
type GetPacketCapturesPcapOK struct {
	Payload io.Writer
}

func (o *GetPacketCapturesPcapOK) Error() string {
	return fmt.Sprintf("[POST /packet/captures/x/merge][%d] getPacketCapturesPcapOK  %+v", 200, o.Payload)
}
func (o *GetPacketCapturesPcapOK) GetPayload() io.Writer {
	return o.Payload
}

func (o *GetPacketCapturesPcapOK) readResponse(response runtime.ClientResponse, consumer runtime.Consumer, formats strfmt.Registry) error {

	// response payload
	if err := consumer.Consume(response.Body(), o.Payload); err != nil && err != io.EOF {
		return err
	}

	return nil
}

/*GetPacketCapturesPcapBody GetPacketCapturesPcapConfig
//
// Parameters for the capture data retrieval
swagger:model GetPacketCapturesPcapBody
*/
type GetPacketCapturesPcapBody struct {

	// List of capture results identifiers
	// Required: true
	// Min Items: 1
	Ids []string `json:"ids"`

	// The packet offset in the capture buffer to end reading (0 based)
	// Minimum: 0
	PacketEnd *int64 `json:"packet_end,omitempty"`

	// The packet offset in the capture buffer to start reading (0 based)
	// Minimum: 0
	PacketStart *int64 `json:"packet_start,omitempty"`
}

// Validate validates this get packet captures pcap body
func (o *GetPacketCapturesPcapBody) Validate(formats strfmt.Registry) error {
	var res []error

	if err := o.validateIds(formats); err != nil {
		res = append(res, err)
	}

	if err := o.validatePacketEnd(formats); err != nil {
		res = append(res, err)
	}

	if err := o.validatePacketStart(formats); err != nil {
		res = append(res, err)
	}

	if len(res) > 0 {
		return errors.CompositeValidationError(res...)
	}
	return nil
}

func (o *GetPacketCapturesPcapBody) validateIds(formats strfmt.Registry) error {

	if err := validate.Required("config"+"."+"ids", "body", o.Ids); err != nil {
		return err
	}

	iIdsSize := int64(len(o.Ids))

	if err := validate.MinItems("config"+"."+"ids", "body", iIdsSize, 1); err != nil {
		return err
	}

	return nil
}

func (o *GetPacketCapturesPcapBody) validatePacketEnd(formats strfmt.Registry) error {
	if swag.IsZero(o.PacketEnd) { // not required
		return nil
	}

	if err := validate.MinimumInt("config"+"."+"packet_end", "body", *o.PacketEnd, 0, false); err != nil {
		return err
	}

	return nil
}

func (o *GetPacketCapturesPcapBody) validatePacketStart(formats strfmt.Registry) error {
	if swag.IsZero(o.PacketStart) { // not required
		return nil
	}

	if err := validate.MinimumInt("config"+"."+"packet_start", "body", *o.PacketStart, 0, false); err != nil {
		return err
	}

	return nil
}

// ContextValidate validates this get packet captures pcap body based on context it is used
func (o *GetPacketCapturesPcapBody) ContextValidate(ctx context.Context, formats strfmt.Registry) error {
	return nil
}

// MarshalBinary interface implementation
func (o *GetPacketCapturesPcapBody) MarshalBinary() ([]byte, error) {
	if o == nil {
		return nil, nil
	}
	return swag.WriteJSON(o)
}

// UnmarshalBinary interface implementation
func (o *GetPacketCapturesPcapBody) UnmarshalBinary(b []byte) error {
	var res GetPacketCapturesPcapBody
	if err := swag.ReadJSON(b, &res); err != nil {
		return err
	}
	*o = res
	return nil
}