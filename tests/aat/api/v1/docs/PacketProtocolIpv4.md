# PacketProtocolIpv4

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**version** | **int** | IP header version | [optional] [default to 4]
**header_length** | **int** | IP header length | [optional] [default to 20]
**tos** | **int** | Type of Service field | [optional] [default to 0]
**packet_length** | **int** | IP packet length (include payload) | [optional] 
**id** | **int** | Identification field | [optional] 
**flags** | **list[str]** | IP header flags | [optional] 
**fragment_offset** | **int** | IP fragment offset | [optional] [default to 0]
**ttl** | **int** | Time To Live field | [optional] 
**protocol** | **int** | Protocol field | [optional] 
**checksum** | **int** | IPv4 header checksum | [optional] 
**source** | [**Ipv4Address**](Ipv4Address.md) |  | [optional] 
**destination** | [**Ipv4Address**](Ipv4Address.md) |  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


