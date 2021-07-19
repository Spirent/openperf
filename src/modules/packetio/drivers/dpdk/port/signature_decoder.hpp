#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_DECODER_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_signature_decoder
    : public rx_callback<callback_signature_decoder>
{
    using parent_type = rx_callback<callback_signature_decoder>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
    void* callback_arg() const;

    using payload_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<const uint8_t*, std::byte[utils::signature_length]>>;

    using sig_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<uint32_t, uint32_t, uint64_t, int>>;

    struct alignas(RTE_CACHE_LINE_SIZE) scratch_t
    {
        time_t epoch_offset;
        payload_container payloads;
        std::array<int, utils::chunk_size> crc_matches;
        sig_container signatures;
    };

    mutable std::vector<scratch_t> scratch;
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
