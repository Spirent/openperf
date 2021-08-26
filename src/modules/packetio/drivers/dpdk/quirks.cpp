#include "packetio/drivers/dpdk/names.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/quirks.hpp"
#include "utils/associative_array.hpp"

namespace openperf::packetio::dpdk::quirks {

namespace detail {

constexpr auto adjust_rx_pktlens =
    utils::associative_array<std::string_view, uint16_t>(
        std::pair(driver_names::mlx5, RTE_PKTMBUF_HEADROOM));

constexpr auto sane_rx_pktlens =
    utils::associative_array<std::string_view, uint32_t>(
        std::pair(driver_names::ring, RTE_MBUF_DEFAULT_BUF_SIZE));

constexpr auto slow_rx_offloads =
    utils::associative_array<std::string_view, uint64_t>(
        std::pair(driver_names::mlx5, DEV_RX_OFFLOAD_SCATTER),
        std::pair(driver_names::virtio, DEV_RX_OFFLOAD_RSS_HASH));

constexpr auto slow_tx_offloads =
    utils::associative_array<std::string_view, uint64_t>(
        std::pair(driver_names::virtio,
                  DEV_TX_OFFLOAD_TCP_CKSUM | DEV_TX_OFFLOAD_UDP_CKSUM));

} // namespace detail

uint16_t adjust_max_rx_pktlen(uint16_t port_id)
{
    return (utils::key_to_value(detail::adjust_rx_pktlens,
                                port_info::driver_name(port_id))
                .value_or(0));
}

uint32_t sane_max_rx_pktlen(uint16_t port_id)
{
    return (utils::key_to_value(detail::sane_rx_pktlens,
                                port_info::driver_name(port_id))
                .value_or(1024 * 9));
}

uint64_t slow_rx_offloads(uint16_t port_id)
{
    return (utils::key_to_value(detail::slow_rx_offloads,
                                port_info::driver_name(port_id))
                .value_or(0));
}

uint64_t slow_tx_offloads(uint16_t port_id)
{
    return (utils::key_to_value(detail::slow_tx_offloads,
                                port_info::driver_name(port_id))
                .value_or(0));
}

} // namespace openperf::packetio::dpdk::quirks
