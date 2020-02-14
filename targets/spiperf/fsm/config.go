package fsm

import "net/url"

// Configuration repository of information for a test.
type Configuration struct {
	OpenperfURL *url.URL

	UpstreamRateBps   uint64
	DownstreamRateBps uint64

	TransmitDuration uint
	//XXX: string type is a placeholder. Per PR review suggestion:
	//"replace this with a type, e.g. TestDuration and write code like
	//func ParseTestDuration(s string) (TestDuration, error) with associated unit tests"
	DurationUnits string

	FixedFrameSize uint

	IMIXGenomeCode *string //XXX: double-check what the REST API is expecting (array vs single string)

	ClientAddresses *AddressConfiguration
	ServerAddresses *AddressConfiguration
}

// AddressConfiguration store list of protocol headers and associated addresses.
type AddressConfiguration struct {
	ProtocolList []string
	AddressList  []string
}
