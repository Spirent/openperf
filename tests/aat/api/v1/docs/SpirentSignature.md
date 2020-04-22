# SpirentSignature

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**stream_id** | **int** | Stream IDs are created for each flow of the definition. This property specifies the ID to use for the first flow. Subsequent flows will use incremented IDs. For example, if a traffic definitions contains 20 flows with a first_stream_id value of 1, then the definition will use 1-20 for stream ids.  | 
**fill** | [**SpirentSignatureFill**](SpirentSignatureFill.md) |  | [optional] 
**latency** | **str** | Indicates timestamp offset | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


