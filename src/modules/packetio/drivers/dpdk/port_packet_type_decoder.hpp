#ifndef _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_

#include <map>

#include "packetio/drivers/dpdk/port_feature_toggle.hpp"

struct rte_eth_rxtx_callback;

namespace openperf::packetio::dpdk::port {

class callback_packet_type_decoder
{
public:
    callback_packet_type_decoder(uint16_t port_id);
    ~callback_packet_type_decoder();

    callback_packet_type_decoder(callback_packet_type_decoder&&) noexcept;
    callback_packet_type_decoder&
    operator=(callback_packet_type_decoder&&) noexcept;

    uint16_t port_id() const;

    void enable();
    void disable();

    using callback_map = std::map<uint16_t, const rte_eth_rxtx_callback*>;

private:
    callback_map m_callbacks;
    uint16_t m_port;
};

struct packet_type_decoder
    : port_feature_toggle<packet_type_decoder,
                          callback_packet_type_decoder,
                          null_feature>
{
    variant_type feature;
    packet_type_decoder(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_ */
