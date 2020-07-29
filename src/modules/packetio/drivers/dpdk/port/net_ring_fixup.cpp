#include <cstring>

#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/port/net_ring_fixup.hpp"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"

namespace openperf::packetio::dpdk::port {

std::string callback_net_ring_fixup::description()
{
    return ("net_ring metadata fixup");
}

static uint16_t fixup_net_ring_metadata([[maybe_unused]] uint16_t port_id,
                                        [[maybe_unused]] uint16_t queue_id,
                                        rte_mbuf* packets[],
                                        uint16_t nb_packets,
                                        [[maybe_unused]] uint16_t max_packets,
                                        [[maybe_unused]] void* user_param)
{
    std::for_each(packets, packets + nb_packets, mbuf_clear_tx_sink);
    return (nb_packets);
}

rx_callback<callback_net_ring_fixup>::rx_callback_fn
callback_net_ring_fixup::callback()
{
    return (fixup_net_ring_metadata);
}

static net_ring_fixup::variant_type make_net_ring_fixup(uint16_t port_id)
{
    if (std::strcmp(model::port_info(port_id).driver_name(), "net_ring") == 0) {
        return (callback_net_ring_fixup(port_id));
    }

    return (null_feature(port_id));
}

net_ring_fixup::net_ring_fixup(uint16_t port_id)
    : feature(make_net_ring_fixup(port_id))
{}

} // namespace openperf::packetio::dpdk::port
