# PacketGeneratorConfig

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**duration** | [**TrafficDuration**](TrafficDuration.md) |  | 
**flow_count** | **int** | Specifies the total number of flows in all traffic definitions | [optional] 
**load** | [**TrafficLoad**](TrafficLoad.md) |  | 
**order** | **str** | Tells the generator how to sequence multiple traffic definitions. Only needed if more than one traffic definition is present.  | [optional] 
**protocol_counters** | **list[str]** | List of protocol counters to update per transmitted packet.  | [optional] 
**traffic** | [**list[TrafficDefinition]**](TrafficDefinition.md) | List of traffic definitions | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


