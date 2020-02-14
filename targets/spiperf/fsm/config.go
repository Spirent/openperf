package fsm

type Configuration struct {
	OpenperfURL string

	TransmitDirection string //FIXME: make this an enum {up, down, both}. Do we even need this?

	UpstreamRate   uint
	DownstreamRate uint

	TransmitDuration uint
	DurationUnits    string

	// FixedFrameSize points to a fixed value. Should be nil when using IMIX mode.
	FixedFrameSize        *uint
	FrameSizeDistribution string

	IMIXGenomeCode *string //FIXME double-check what the REST API is expecting (array vs single string)

	ClientAddresses *AddressConfiguration
	ServerAddresses *AddressConfiguration
}

type AddressConfiguration struct {
	ProtocolList []string
	AddressList  []string
}
