# PacketGenerator

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique generator identifier | 
**target_id** | **str** | Specifies the unique target for packets from this generator. This id may refer to either a port or an interface id.  | 
**active** | **bool** | Indicates whether this object is currently generating packets or not.  | 
**learning** | **str** | Current state of MAC learning. For generators targeted to interfaces this must be \&quot;resolved\&quot; else generator won&#39;t start.  | 
**config** | [**PacketGeneratorConfig**](PacketGeneratorConfig.md) |  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


