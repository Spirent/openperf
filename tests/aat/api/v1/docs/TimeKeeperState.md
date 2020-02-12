# TimeKeeperState

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**frequency** | **float** | The time counter frequency as measured by the interval between the two best timestamp exchanges with the time source over the past two hours, in Hz.  | [optional] 
**frequency_error** | **float** | The estimated error of the time counter frequency measurement, in Hz. | [optional] 
**local_frequency** | **float** | The time counter frequency as measured by the interval between the two best timestamp exchanges with the time source over the past hour, in Hz. This value is used to help determine time stamp error due to time counter frequency drift.  | [optional] 
**local_frequency_error** | **float** | The estimated error of the local time counter frequency measurement, in Hz. | [optional] 
**offset** | **float** | The offset applied to time counter derived timestamp values, in seconds.  This value comes from the system host clock.  | 
**synced** | **bool** | The time keeper is considered to be synced to the time source if a clock offset, theta, has been calculated and applied within the past 20 minutes.  | 
**theta** | **float** | The calculated correction to apply to the offset, based on the measured time counter frequency and time source timestamps.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


