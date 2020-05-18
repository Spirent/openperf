#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_

#include <map>

#include "packetio/drivers/dpdk/port_feature_toggle.hpp"

struct rte_eth_rxtx_callback;

namespace openperf::packetio::dpdk::port {

class callback_signature_encoder
{
public:
    callback_signature_encoder(uint16_t port_id);
    ~callback_signature_encoder();

    callback_signature_encoder(callback_signature_encoder&&) noexcept;
    callback_signature_encoder&
    operator=(callback_signature_encoder&&) noexcept;

    uint16_t port_id() const;

    void enable();
    void disable();

    using callback_map = std::map<uint16_t, const rte_eth_rxtx_callback*>;

private:
    callback_map m_callbacks;
    uint16_t m_port;
};

struct signature_encoder
    : port_feature_toggle<signature_encoder,
                          callback_signature_encoder,
                          null_feature>
{
    variant_type feature;
    signature_encoder(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_ */
