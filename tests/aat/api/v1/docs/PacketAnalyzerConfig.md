# PacketAnalyzerConfig

## Properties
Name | Type | Description | Notes
------------ | ------------- | ------------- | -------------
**filter** | **str** | Berkley Packet Filter (BPF) rules that matches input packets for this analyzer to count. An empty rule, the default, matches all frames.  | [optional] 
**protocol_counters** | **list[str]** | List of protocol counters to update per analyzer for received packets.  | 
**flow_counters** | **list[str]** | List of results to generate per flow for received packets. Sequencing, latency, and jitter results require Spirent signatures in the received packets. Pseudo Random Bit Sequence (PRBS) results require packet payloads to contain compatible PRBS data.  | 
**flow_digests** | **list[str]** | List of result digests to generate per flow for received packets. Sequence run length, latency, and jitter digests require Spirent signatures in the received packets.  | [optional] 

[[Back to Model list]](../README.md#documentation-for-models) [[Back to API list]](../README.md#documentation-for-api-endpoints) [[Back to README]](../README.md)


