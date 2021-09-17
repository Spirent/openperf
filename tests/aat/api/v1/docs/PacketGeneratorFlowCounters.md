# PacketGeneratorFlowCounters

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**errors** | **int** | The number of packets not transmitted due to errors | 
**octets_actual** | **int** | The total number of octets that have been transmitted | 
**octets_dropped** | **int** | The total number of octets that were dropped due to overrunning the transmit queue. Transmit packet drops are not enabled by default and must be explicitly enabled.  | [optional] 
**octets_intended** | **int** | The total number of octets that should have been transmitted | 
**packets_actual** | **int** | The total number of packets that have been transmitted | 
**packets_dropped** | **int** | The total number of packets that were dropped due to overrunning the transmit queue. Transmit packet drops are not enabled by default and must be explicitly enabled.  | [optional] 
**packets_intended** | **int** | The total number of packets that should have been transmitted | 
**timestamp_first** | **datetime** | The timestamp of the first transmitted packet | [optional] 
**timestamp_last** | **datetime** | The timestamp of the most recently transmitted packet | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


