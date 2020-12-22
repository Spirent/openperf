# PacketGenerator

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique generator identifier | 
**target_id** | **str** | Specifies the unique target for packets from this generator. This id may refer to either a port or an interface id.  | 
**active** | **bool** | Indicates whether this object is currently generating packets or not.  | 
**learning_resolved** | **bool** | Indicates whether this object resolved all destination MAC address. If present and false this object cannot start.  | [optional] 
**config** | [**PacketGeneratorConfig**](PacketGeneratorConfig.md) |  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


