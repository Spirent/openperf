# TDigestResult

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | T-Digest configuration unique identifier | 
**function** | **str** | The function to apply to the statistic before evaluating | 
**stat_x** | **str** | The X statistic to track | 
**stat_y** | **str** | The Y statistic to track (when using DXDY function) | [optional] 
**timestamp** | **datetime** | The ISO8601-formatted date of the last result update | 
**centroids** | [**list[TDigestCentroid]**](TDigestCentroid.md) | Array of centroids | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)

