#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_PAYLOAD_FILLER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_PAYLOAD_FILLER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_signature_payload_filler
    : public tx_callback<callback_signature_payload_filler>
{
    using parent_type = tx_callback<callback_signature_payload_filler>;

    using parent_type::tx_callback;
    static std::string description();
    static parent_type::tx_callback_fn callback();
    void* callback_arg() const;

    using const_fill_args = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<uint8_t*, uint16_t, uint16_t>>;

    using incr_fill_args =
        openperf::utils::soa_container<utils::chunk_array,
                                       std::tuple<uint8_t*, uint16_t, uint8_t>>;

    using decr_fill_args =
        openperf::utils::soa_container<utils::chunk_array,
                                       std::tuple<uint8_t*, uint16_t, uint8_t>>;

    using prbs_fill_args =
        openperf::utils::soa_container<utils::chunk_array,
                                       std::tuple<uint8_t*, uint16_t>>;

    struct alignas(RTE_CACHE_LINE_SIZE) scratch_t
    {
        const_fill_args const_args;
        incr_fill_args incr_args;
        decr_fill_args decr_args;
        prbs_fill_args prbs_args;
        uint32_t prbs_seed = std::numeric_limits<uint32_t>::max();
    };

    mutable std::vector<scratch_t> scratch;
};

struct signature_payload_filler
    : feature_toggle<signature_payload_filler,
                     callback_signature_payload_filler,
                     null_feature>
{
    variant_type feature;
    signature_payload_filler(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_PAYLOAD_FILLER_HPP_ */
