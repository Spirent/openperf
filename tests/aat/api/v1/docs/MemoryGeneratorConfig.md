# MemoryGeneratorConfig

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**buffer_size** | **int** | Memory size constraint. The buffer can never be larger than the value specified (in bytes) | 
**pre_allocate_buffer** | **bool** | Initialize allocation process for the memory buffer | 
**reads_per_sec** | **int** | Number of read operations to perform per second | 
**read_size** | **int** | Number of bytes to use for each read operation | 
**read_threads** | **int** | Number of read worker threads | 
**writes_per_sec** | **int** | Number of write operations to perform per second | 
**write_size** | **int** | Number of bytes to use for each write operation | 
**write_threads** | **int** | Number of write worker threads | 
**pattern** | **str** | IO access pattern | 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


