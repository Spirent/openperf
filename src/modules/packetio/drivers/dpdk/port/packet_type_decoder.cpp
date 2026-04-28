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
#if (RTE_VERSION >= RTE_VERSION_NUM(23, 11, 7, 0)                              \
     && RTE_VERSION < RTE_VERSION_NUM(24, 11, 6, 0))
            /*
               There is a known issue with rte_net_get_ptype() in 23.11
               and 24.11 branches which causes it to return
               RTE_PTYPE_L2_ETHER_QINQ for normal VLAN packets.
               https://mails.dpdk.org/archives/stable/2026-April/057407.html

               This code works around the issue by verifying the ether type
               in the header matches VLAN. If it does, the packet type is
               updated to RTE_PTYPE_L2_ETHER_VLAN.
            */
            mbuf->packet_type = rte_net_get_ptype(mbuf, nullptr, decode_mask);
            if ((mbuf->packet_type & RTE_PTYPE_L2_MASK)
                == RTE_PTYPE_L2_ETHER_QINQ) {
                struct rte_ether_hdr eh_copy;
                auto eh = reinterpret_cast<const struct rte_ether_hdr*>(
                    rte_pktmbuf_read(mbuf, 0, sizeof(eh_copy), &eh_copy));
                if ((eh != nullptr)
                    && (eh->ether_type
                        == rte_cpu_to_be_16(RTE_ETHER_TYPE_VLAN))) {
                    mbuf->packet_type = (mbuf->packet_type & ~RTE_PTYPE_L2_MASK)
                                        | RTE_PTYPE_L2_ETHER_VLAN;
                }
            }
#elif RTE_VERSION >= RTE_VERSION_NUM(24, 11, 6, 0)
#error Verify rte_net_get_ptype() detects VLAN correctly! If so, remove workaround code above.
#else
            mbuf->packet_type = rte_net_get_ptype(mbuf, nullptr, decode_mask);
#endif
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
