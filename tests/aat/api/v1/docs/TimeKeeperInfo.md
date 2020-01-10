# TimeKeeperInfo

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**frequency** | **float** | The TimeCounter frequency as measured by the interval between the two best timestamp exchanges with the TimeSource over the past two hours, in hz.  | [optional] 
**frequency_error** | **float** | The estimated error in the TimeCounter frequency measurement, in hz. | [optional] 
**local_frequency** | **float** | The TimeCounter frequency as measured by the interval between the two best timestamp exchanges with the TimeSource over the past hour, in hz. This value is used to help determine TimeCounter drift.  | [optional] 
**local_frequency_error** | **float** | The estimated error in the local TimeCounter frequency measurement, in hz. | [optional] 
**offset** | **float** | The offset applied to TimeCounter derived timestamp values, in seconds.  This value comes from the system host clock.  | 
**synced** | **bool** | Indicates if the clock is synchronized to the source | [optional] 
**theta** | **float** | The calculated correction to apply to the offset, based on the measured TimeCounter frequency and TimeSource timestamps.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


