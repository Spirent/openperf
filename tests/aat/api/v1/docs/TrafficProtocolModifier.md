# TrafficProtocolModifier

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**name** | **str** | Packet protocol field to modify. Context determines what field names are valid and what data is expected in the modifier.  | 
**permute** | **bool** | Specifies whether to pseudo-randomly order the modifier values  | 
**field** | [**TrafficProtocolFieldModifier**](TrafficProtocolFieldModifier.md) |  | [optional] 
**ipv4** | [**TrafficProtocolIpv4Modifier**](TrafficProtocolIpv4Modifier.md) |  | [optional] 
**ipv6** | [**TrafficProtocolIpv6Modifier**](TrafficProtocolIpv6Modifier.md) |  | [optional] 
**mac** | [**TrafficProtocolMacModifier**](TrafficProtocolMacModifier.md) |  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


