#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_signature_encoder
    : public tx_callback<callback_signature_encoder>
{
    using parent_type = tx_callback<callback_signature_encoder>;

    using parent_type::tx_callback;
    static std::string description();
    static parent_type::tx_callback_fn callback();
    static void* callback_arg();
};

struct signature_encoder
    : feature_toggle<signature_encoder,
                     callback_signature_encoder,
                     null_feature>
{
    variant_type feature;
    signature_encoder(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_ */
