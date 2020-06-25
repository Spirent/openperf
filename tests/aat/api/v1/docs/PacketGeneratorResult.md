# PacketGeneratorResult

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique generator result identifier | 
**generator_id** | **str** | Unique generator identifier that produced this result | [optional] 
**active** | **bool** | Indicates whether this result is currently being updated | 
**flow_counters** | [**PacketGeneratorFlowCounters**](PacketGeneratorFlowCounters.md) |  | 
**flows** | **list[str]** | List of unique flow ids included in stats. Individual flow statistics may be queried via the &#x60;tx-flows&#x60; endpoint.  | 
**protocol_counters** | [**PacketGeneratorProtocolCounters**](PacketGeneratorProtocolCounters.md) |  | 
**remaining** | [**TrafficDurationRemainder**](TrafficDurationRemainder.md) |  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


