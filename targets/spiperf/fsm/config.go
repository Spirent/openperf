package fsm

// Configuration repository of information for a spiperf test.
type Configuration struct {
	OpenperfURL string

	UpstreamRate   uint
	DownstreamRate uint

	TransmitDuration uint
	DurationUnits    string

	// FixedFrameSize points to a fixed value. Should be nil when using IMIX mode.
	FixedFrameSize        *uint
	FrameSizeDistribution string

	IMIXGenomeCode *string //XXX: double-check what the REST API is expecting (array vs single string)

	ClientAddresses *AddressConfiguration
	ServerAddresses *AddressConfiguration
}

// AddressConfiguration store list of protocol headers and associated addresses.
type AddressConfiguration struct {
	ProtocolList []string
	AddressList  []string
}
