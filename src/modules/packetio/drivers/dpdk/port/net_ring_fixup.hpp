#ifndef _OP_PACKETIO_DPDK_PORT_NET_RING_FIXUP_HPP_
#define _OP_PACKETIO_DPDK_PORT_NET_RING_FIXUP_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_net_ring_fixup : public rx_callback<callback_net_ring_fixup>
{
    using parent_type = rx_callback<callback_net_ring_fixup>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
};

struct net_ring_fixup
    : feature_toggle<net_ring_fixup, callback_net_ring_fixup, null_feature>
{
    variant_type feature;
    net_ring_fixup(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_NET_RING_FIXUP_HPP_ */
