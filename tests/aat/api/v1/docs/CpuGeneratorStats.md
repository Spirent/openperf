# CpuGeneratorStats

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**available** | **int** | The total amount of CPU time available | 
**utilization** | **int** | The amount of CPU time used | 
**target** | **int** | The amount of target CPU time | 
**system** | **int** | The amount of system time used, e.g. kernel or system calls | 
**user** | **int** | The amount of user time used, e.g. openperf code | 
**steal** | **int** | The amount of time the hypervisor reported our virtual cores were blocked | [optional] 
**error** | **int** | The difference between intended and actual CPU utilization | 
**cores** | [**list[CpuGeneratorCoreStats]**](CpuGeneratorCoreStats.md) | Statistics of the CPU cores (in the order they were specified in generator configuration) | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


