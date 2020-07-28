# TvlpConfiguration

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique TVLP configuration identifier | 
**state** | **str** | TVLP configuration state - ready - TVLP contains a valid configuration - countdown - TVLP has been given a start time in the future and is waiting to start replaying a profile - running - TVLP is replaying a profile - error - TVLP encountered a runtime error  | 
**time** | [**TvlpConfigurationTime**](TvlpConfigurationTime.md) |  | [optional] 
**profile** | [**TvlpProfile**](TvlpProfile.md) |  | 
**error** | **str** | string describing error condition; only when state &#x3D;&#x3D; error | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


