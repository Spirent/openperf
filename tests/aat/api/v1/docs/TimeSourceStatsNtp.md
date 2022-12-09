# TimeSourceStatsNtp

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**last_rx_accepted** | **datetime** | the time and date of the last accepted NTP reply, in ISO8601 format | [optional] 
**last_rx_ignored** | **datetime** | The time and date of the last ignored NTP reply, in ISO8601 format | [optional] 
**poll_period** | **int** | Current NTP server poll period, in seconds | 
**rx_ignored** | **int** | Received packets that were ignored due to an invalid origin timestamp or stratum, e.g. a Kiss-o&#39;-Death packet  | 
**rx_packets** | **int** | Received packets | 
**tx_packets** | **int** | Transmitted packets | 
**stratum** | **int** | Time source distance from a NTP reference clock, in network hops.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


