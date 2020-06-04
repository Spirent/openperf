#ifndef _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_
#define _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_timestamper : public rx_callback<callback_timestamper>
{
    using parent_type = rx_callback<callback_timestamper>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
};

struct timestamper
    : feature_toggle<timestamper, callback_timestamper, null_feature>
{
    variant_type feature;
    timestamper(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_ */
