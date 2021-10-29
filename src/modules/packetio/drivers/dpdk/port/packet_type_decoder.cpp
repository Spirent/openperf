#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/port/packet_type_decoder.hpp"
#include "utils/prefetch_for_each.hpp"

namespace openperf::packetio::dpdk::port {

inline constexpr uint32_t decode_mask =
    RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK;

static uint16_t decode_packet_types([[maybe_unused]] uint16_t port_id,
                                    [[maybe_unused]] uint16_t queue_id,
                                    rte_mbuf* packets[],
                                    uint16_t nb_packets,
                                    [[maybe_unused]] uint16_t max_packets,
                                    [[maybe_unused]] void* user_param)
{
    utils::prefetch_for_each(
        packets,
        packets + nb_packets,
        [](const auto* mbuf) { rte_prefetch0(rte_pktmbuf_mtod(mbuf, void*)); },
        [](auto* mbuf) {
            mbuf->packet_type = rte_net_get_ptype(mbuf, nullptr, decode_mask);
        },
        mbuf_prefetch_offset);

    return (nb_packets);
}

std::string callback_packet_type_decoder::description()
{
    return ("packet type decoding");
}

rx_callback<callback_packet_type_decoder>::rx_callback_fn
callback_packet_type_decoder::callback()
{
    return (decode_packet_types);
}

static packet_type_decoder::variant_type
make_packet_type_decoder(uint16_t port_id)
{
    if (rte_eth_dev_get_supported_ptypes(port_id, decode_mask, nullptr, 0)
        <= 0) {
        return (callback_packet_type_decoder(port_id));
    }

    return (null_feature(port_id));
}

packet_type_decoder::packet_type_decoder(uint16_t port_id)
    : feature(make_packet_type_decoder(port_id))
{}

} // namespace openperf::packetio::dpdk::port
