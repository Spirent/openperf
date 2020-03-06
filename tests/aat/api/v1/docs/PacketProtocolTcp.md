# PacketProtocolTcp

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**source_port** | [**TcpIpPort**](TcpIpPort.md) |  | 
**destination_port** | [**TcpIpPort**](TcpIpPort.md) |  | 
**sequence_number** | **int** | sequence number field | [optional] 
**ack_number** | **int** | ACK number field | [optional] 
**data_offset** | **int** | offset to data from the start of the TCP header | [optional] 
**flags** | **list[str]** | TCP header flags | [optional] 
**window** | **int** | sequence window field | [optional] 
**checksum** | **int** | checksum field | [optional] 
**urgent_pointer** | **int** | urgent pointer field | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


