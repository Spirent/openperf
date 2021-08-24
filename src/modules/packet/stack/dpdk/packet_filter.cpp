#include "packet/stack/dpdk/packet_filter.hpp"

namespace openperf::packet::stack::dpdk {

namespace impl {

bool bpf_filter::operator()(const rte_mbuf* packet) const noexcept
{
    const packetio::packet::packet_buffer* const input[] = {
        reinterpret_cast<const packetio::packet::packet_buffer*>(packet)};
    const packetio::packet::packet_buffer* output[] = {nullptr};

    /* filter_burst returns the number of matches */
    return (
        const_cast<packet::bpf::bpf&>(filter).filter_burst(input, output, 1));
}

} // namespace impl

static packet_filter::filter_strategy
make_filter_strategy(const std::optional<std::string>& filter)
{
    if (!filter) { return (impl::accept_filter{}); }

    return (impl::bpf_filter{packet::bpf::bpf(filter.value())});
}

packet_filter::packet_filter(const std::optional<std::string>& filter)
    : m_filter(make_filter_strategy(filter))
{}

} // namespace openperf::packet::stack::dpdk
