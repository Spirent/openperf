#ifndef _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_
#define _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_

#include <map>

#include "packetio/drivers/dpdk/port_feature_toggle.hpp"

struct rte_eth_rxtx_callback;

namespace openperf::packetio::dpdk::port {

class callback_timestamper
{
public:
    callback_timestamper(uint16_t port_id);
    ~callback_timestamper();

    callback_timestamper(callback_timestamper&&);
    callback_timestamper& operator=(callback_timestamper&&);

    uint16_t port_id() const;

    void enable();
    void disable();

    using callback_map = std::map<uint16_t, const rte_eth_rxtx_callback*>;

private:
    callback_map m_callbacks;
    uint16_t m_port;
};

struct timestamper
    : port_feature_toggle<timestamper, callback_timestamper, null_feature>
{
    variant_type feature;
    timestamper(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_TIMESTAMPER_HPP_ */
