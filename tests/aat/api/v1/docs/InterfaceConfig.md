# InterfaceConfig

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**protocols** | [**list[InterfaceProtocolConfig]**](InterfaceProtocolConfig.md) | A stack of protocol configurations, beginning with the outermost protocol (i.e. closest to the physical port)  | 
**rx_filter** | **str** | Berkley Packet Filter (BPF) rules that matches input packets for this interface. An empty rule, the default, matches all packets.  | [optional] 
**tx_filter** | **str** | Berkley Packet Filter (BPF) rules that matches output packets for this interface. An empty rule, the default, matches all packets.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


