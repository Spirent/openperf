#ifndef _OP_PACKETIO_DPDK_PORT_CHECKSUM_CALCULATOR_HPP_
#define _OP_PACKETIO_DPDK_PORT_CHECKSUM_CALCULATOR_HPP_

#include "packetio/drivers/dpdk/port/callback.hpp"
#include "packetio/drivers/dpdk/port/feature_toggle.hpp"
#include "packetio/drivers/dpdk/port/signature_utils.hpp"
#include "utils/soa_container.hpp"

namespace libpacket::protocol {
struct ipv4;
struct ipv6;
} // namespace libpacket::protocol

namespace openperf::packetio::dpdk::port {

struct callback_checksum_calculator
    : public tx_callback<callback_checksum_calculator>
{
    using parent_type = tx_callback<callback_checksum_calculator>;

    using parent_type::tx_callback;
    static std::string description();
    static parent_type::tx_callback_fn callback();
    void* callback_arg() const;

    /*
     * A container of header/checksum pairs.
     */
    using ipv4_cksum_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<libpacket::protocol::ipv4*, uint32_t>>;

    /*
     * We need a little more information for IPv6 packets due to
     * possible IPv6 extension headers between the header and
     * the payload:
     * 0: ipv6 header ptr
     * 1: payload ptr
     * 2: protocol
     * 3: checksum
     */
    using ipv6_cksum_container = openperf::utils::soa_container<
        utils::chunk_array,
        std::tuple<libpacket::protocol::ipv6*, uint8_t*, uint8_t, uint32_t>>;

    struct scratch_t
    {
        ipv4_cksum_container ipv4_cksums;
        ipv4_cksum_container ipv4_tcpudp_cksums;
        ipv6_cksum_container ipv6_tcpudp_cksums;
    };

    mutable scratch_t scratch;
};

struct checksum_calculator
    : feature_toggle<checksum_calculator,
                     callback_checksum_calculator,
                     null_feature>
{
    variant_type feature;
    checksum_calculator(uint16_t port_id);
};

} // namespace openperf::packetio::dpdk::port

#endif /* _OP_PACKETIO_DPDK_PORT_CHECKSUM_CALCULATOR_HPP_ */
