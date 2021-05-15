# TimeKeeperStats

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**frequency_accept** | **int** | The number of times the frequency calculation has been updated. | 
**frequency_reject** | **int** | The number of times the frequency calculation has been rejected due to an excessive delta between old and new values.  | 
**local_frequency_accept** | **int** | The number of times the local frequency calculation has been updated. | 
**local_frequency_reject** | **int** | The number of times the local frequency calculation has been rejected due to an excessive delta between old and new values.  | 
**round_trip_times** | [**TimeKeeperStatsRoundTripTimes**](TimeKeeperStatsRoundTripTimes.md) |  | 
**theta_accept** | **int** | The number of times the theta calculation has been updated. | 
**theta_reject** | **int** | Then umber of times the theta calculation has been rejected due to excessive delta between old and new values.  | 
**timestamps** | **int** | The number of timestamps in the current working set of timestamps. Old timestamps are dropped from the history of timestamps as they become irrelevant.  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


