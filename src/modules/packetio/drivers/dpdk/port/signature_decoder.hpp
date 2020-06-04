#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_

#include <map>

#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

struct rte_eth_rxtx_callback;

namespace openperf::packetio::dpdk::port {

class callback_signature_decoder
{
public:
    callback_signature_decoder(uint16_t port_id);
    ~callback_signature_decoder();

    callback_signature_decoder(callback_signature_decoder&&) noexcept;
    callback_signature_decoder&
    operator=(callback_signature_decoder&&) noexcept;

    uint16_t port_id() const;

    void enable();
    void disable();

    using callback_map = std::map<uint16_t, const rte_eth_rxtx_callback*>;

private:
    callback_map m_callbacks;
    uint16_t m_port;
};

struct signature_decoder
    : feature_toggle<signature_decoder,
                     callback_signature_decoder,
                     null_feature>
{
    variant_type feature;
    signature_decoder(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_ */
