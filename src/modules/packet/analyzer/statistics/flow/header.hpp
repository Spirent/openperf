#ifndef _OP_ANALYZER_STATISTICS_FLOW_HEADERS_HPP_
#define _OP_ANALYZER_STATISTICS_FLOW_HEADERS_HPP_

#include <array>

#include "packet/analyzer/statistics/common.hpp"
#include "packetio/packet_type.hpp"
#include "utils/memcpy.hpp"

namespace openperf::packet::analyzer::statistics::flow {

/*
 * By default, DPDK reserves 128 bytes for pre-pending packet
 * headers. That seems like a reasonable number, but we adjust it down
 * slightly so that we can fit in our packet type flags.
 */
constexpr static uint16_t max_header_length = 124;

/*
 * Not really a counter, but it does go with the rest of the flow
 * counters.
 */
struct header
{
    using packet_type_flags = packetio::packet::packet_type::flags;

    packet_type_flags flags;
    std::array<uint8_t, max_header_length> data;
};

static_assert(sizeof(header) <= 128, "Header to big!");

uint16_t header_length(header::packet_type_flags flags,
                       const uint8_t pkt[],
                       uint16_t pkt_len);

inline uint16_t header_length(const header& header)
{
    return (
        header_length(header.flags, header.data.data(), header.data.size()));
}

template <typename StatsTuple>
void set_header(StatsTuple& tuple,
                header::packet_type_flags flags,
                const uint8_t pkt[])
{
    if constexpr (!has_type<header, StatsTuple>::value) { return; }

    auto& h = get_counter<header, StatsTuple>(tuple);
    h.flags = flags;
    utils::memcpy(
        h.data.data(), pkt, header_length(flags, pkt, max_header_length));
}

/**
 * Debug methods
 **/

void dump(std::ostream& os, const header& stat);

} // namespace openperf::packet::analyzer::statistics::flow

#endif /* _OP_ANALYZER_STATISTICS_FLOW_HEADERS_HPP_ */
