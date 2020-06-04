#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_signature_decoder
    : public rx_callback<callback_signature_decoder>
{
    using parent_type = rx_callback<callback_signature_decoder>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
    static void* callback_arg();
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
