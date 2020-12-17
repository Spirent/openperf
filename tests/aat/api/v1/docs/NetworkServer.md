# NetworkServer

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique network server identifier | 
**port** | **int** | UDP/TCP port on which to listen for client connections | 
**protocol** | **str** | Protocol which is used to establish client connections | 
**interface** | **str** | Bind server socket to a particular device, specified as interface name (required for dpdk driver) | [optional] 
**stats** | [**NetworkServerStats**](NetworkServerStats.md) |  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


