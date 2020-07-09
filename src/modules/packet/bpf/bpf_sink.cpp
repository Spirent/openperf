
#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_build.hpp"
#include "packet/bpf/bpf_sink.hpp"

namespace openperf::packet::bpf {

using sink_feature_flags = openperf::packetio::packet::sink_feature_flags;

openperf::utils::bit_flags<openperf::packetio::packet::sink_feature_flags>
bpf_sink_feature_flags(uint32_t filter_flags)
{
    openperf::utils::bit_flags<sink_feature_flags> needed{};

    if (filter_flags & openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE) {
        needed |= sink_feature_flags::spirent_signature_decode;
        if (filter_flags & openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR) {
            needed |= sink_feature_flags::spirent_prbs_error_detect;
        }
    }
    if (filter_flags
        & (openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR
           | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
           | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR
           | openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR)) {
        needed |= sink_feature_flags::packet_type_decode;
    }

    return needed;
}

openperf::utils::bit_flags<sink_feature_flags>
bpf_sink_feature_flags(const bpf& bpf)
{
    return bpf_sink_feature_flags(bpf.get_filter_flags());
}

} // namespace openperf::packet::bpf
