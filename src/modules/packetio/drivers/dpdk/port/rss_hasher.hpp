#ifndef _OP_PACKETIO_DPDK_PORT_RSS_HASHER_HPP_
#define _OP_PACKETIO_DPDK_PORT_RSS_HASHER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_rss_hasher final : public rx_callback<callback_rss_hasher>
{
    using rx_callback<callback_rss_hasher>::rx_callback;
    static std::string description();
    static rx_callback<callback_rss_hasher>::rx_callback_fn callback();
};

struct rss_hasher
    : feature_toggle<rss_hasher, callback_rss_hasher, null_feature>
{
    variant_type feature;
    rss_hasher(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_RSS_HASHER_HPP_ */
