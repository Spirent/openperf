#ifndef _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_packet_type_decoder
    : public rx_callback<callback_packet_type_decoder>
{
    using parent_type = rx_callback<callback_packet_type_decoder>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
};

struct packet_type_decoder
    : feature_toggle<packet_type_decoder,
                     callback_packet_type_decoder,
                     null_feature>
{
    variant_type feature;
    packet_type_decoder(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_PACKET_TYPE_DECODER_HPP_ */
