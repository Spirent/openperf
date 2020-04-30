# PacketCaptureConfig

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**mode** | **str** | Capture mode | [default to 'buffer']
**buffer_wrap** | **bool** | Indicates whether capture wraps when it reaches the end of the buffer.  When buffer wrap is enabled capture will continue until capture is stopped with the stop command or a stop trigger.  | [optional] [default to False]
**buffer_size** | **int** | Capture buffer size in bytes. | [default to 16777216]
**packet_size** | **int** | Maximum length of packet to capture. If the packet is larger than the packet size, the packet is truncated. | [optional] 
**filter** | **str** | Berkley Packet Filter (BPF) rules that matches packets to capture.  An empty rule, the default, matches all frames.  | [optional] 
**start_trigger** | **str** | Berkley Packet Filter (BPF) rules used to trigger the start of packet capture.  When a trigger condition is specified, the capture start command puts capture into an armed state and capture will only begin when the trigger condition occurs.  | [optional] 
**stop_trigger** | **str** | Berkley Packet Filter (BPF) rules used to trigger the stop of packet capture.  | [optional] 
**duration** | **int** | Maximum time duration for the capture in msec.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


