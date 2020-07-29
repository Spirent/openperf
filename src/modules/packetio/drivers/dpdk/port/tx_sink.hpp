#ifndef _OP_PACKETIO_DPDK_PORT_TX_SINK_HPP_
#define _OP_PACKETIO_DPDK_PORT_TX_SINK_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_tx_sink : public tx_callback<callback_tx_sink>
{
    using parent_type = tx_callback<callback_tx_sink>;

    using parent_type::tx_callback;
    static std::string description();
    static parent_type::tx_callback_fn callback();
    void* callback_arg() const;
};

struct tx_sink : feature_toggle<tx_sink, callback_tx_sink, null_feature>
{
    using tx_callback_fn = callback_tx_sink::parent_type::tx_callback_fn;

    variant_type feature;
    tx_sink(uint16_t port_id);

    static void set_callback(tx_callback_fn callback);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_TX_SINK_HPP_ */
