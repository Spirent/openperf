#ifndef _OP_PACKETIO_DPDK_PORT_PRBS_ERROR_DETECTOR_HPP_
#define _OP_PACKETIO_DPDK_PORT_PRBS_ERROR_DETECTOR_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "utils/soa_container.hpp"

namespace openperf::packetio::dpdk::port {

struct callback_prbs_error_detector
    : public rx_callback<callback_prbs_error_detector>
{
    using parent_type = rx_callback<callback_prbs_error_detector>;

    using parent_type::rx_callback;
    static std::string description();
    static parent_type::rx_callback_fn callback();
    void* callback_arg() const;

    using prbs_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<const uint8_t*, uint16_t, uint32_t>>;

    struct scratch_t
    {
        utils::chunk_array<rte_mbuf*> prbs_packets;
        prbs_container prbs_segments;
    };

    mutable scratch_t scratch;
};

struct prbs_error_detector
    : feature_toggle<prbs_error_detector,
                     callback_prbs_error_detector,
                     null_feature>
{
    variant_type feature;
    prbs_error_detector(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_PRBS_ERROR_DETECTOR_HPP_ */
