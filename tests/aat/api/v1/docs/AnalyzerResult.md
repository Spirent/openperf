# AnalyzerResult

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**id** | **str** | Unique analyzer result identifier | 
**analyzer_id** | **str** | Unique analyzer identifier that generated this result | [optional] 
**active** | **bool** | Indicates whether the result is currently receiving updates | 
**protocol_counters** | [**AnalyzerProtocolCounters**](AnalyzerProtocolCounters.md) |  | 
**flow_counters** | [**AnalyzerFlowCounters**](AnalyzerFlowCounters.md) |  | 
**flows** | **list[str]** | List of unique flow ids included in stats. Individual flow statistics may be queried via the &#x60;rx-flows&#x60; endpoint.  | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


