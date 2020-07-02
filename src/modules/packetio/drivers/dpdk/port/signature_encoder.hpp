#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_ENCODER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_signature_encoder
    : public tx_callback<callback_signature_encoder>
{
    using parent_type = tx_callback<callback_signature_encoder>;

    using parent_type::tx_callback;
    static std::string description();
    static parent_type::tx_callback_fn callback();
    void* callback_arg() const;

    using sig_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<uint8_t*, uint32_t, uint32_t, uint64_t, int>>;

    using runt_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<const uint8_t*, utils::spirent_signature*>>;

    struct scratch_t
    {
        time_t epoch_offset;
        sig_container signatures;
        std::array<uint32_t, utils::chunk_size> checksums;
        runt_container runts;
    };

    mutable scratch_t scratch;
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
