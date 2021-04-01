# SocketStats

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique socket statistics identifier | [optional] 
**pid** | **int** | Process ID which created the socket | [optional] 
**sid** | **int** | The socket ID (used by server) | [optional] 
**if_index** | **int** | The interface index the socket is bound to | [optional] 
**protocol** | **str** | The socket protocol type | [optional] 
**protocol_id** | **int** | The protocol ID used for raw and packet sockets | [optional] 
**rxq_bytes** | **int** | Number of bytes in the socket receive queue | [optional] 
**txq_bytes** | **int** | Number of bytes in the socket transmit queue | [optional] 
**local_ip_address** | **str** | The local IP address | [optional] 
**remote_ip_address** | **str** | The remote IP address | [optional] 
**local_port** | **int** | The local port number | [optional] 
**remote_port** | **int** | The remote port number | [optional] 
**state** | **str** | The socket state | [optional] 
**send_queue_length** | **int** | The number of packets in the protocol send queue | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


