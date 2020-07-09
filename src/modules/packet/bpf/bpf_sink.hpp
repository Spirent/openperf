#ifndef _OP_PACKET_BPF_SINK_HPP_
#define _OP_PACKET_BPF_SINK_HPP_

#include "packetio/generic_sink.hpp"

namespace openperf::packet::bpf {

class bpf;

openperf::utils::bit_flags<openperf::packetio::packet::sink_feature_flags>
bpf_sink_feature_flags(uint32_t filter_flags);

openperf::utils::bit_flags<openperf::packetio::packet::sink_feature_flags>
bpf_sink_feature_flags(const bpf& bpf);

} // namespace openperf::packet::bpf

#endif // _OP_PACKET_BPF_SINK_HPP_