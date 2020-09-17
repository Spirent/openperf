#include <algorithm>
#include <array>
#include <limits>

#include <net/if.h>

#include "packetio/drivers/dpdk/port_info.hpp"

namespace openperf::packetio::dpdk::port_info {

unsigned socket_id(uint16_t port_id)
{
    return (std::clamp(rte_eth_dev_socket_id(port_id), 0, RTE_MAX_NUMA_NODES));
}

std::string driver_name(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::driver_name));
}

std::string device_name(uint16_t port_id)
{
    auto buffer = std::array<char, RTE_ETH_NAME_MAX_LEN>{};
    if (rte_eth_dev_get_name_by_port(port_id, buffer.data()) == 0) {
        return (std::string(buffer.data()));
    }

    return (std::string{});
}

std::optional<std::string> interface_name(uint16_t port_id)
{
    auto if_index = get_info_field(port_id, &rte_eth_dev_info::if_index);
    auto buffer = std::array<char, IF_NAMESIZE>{};
    if (if_indextoname(if_index, buffer.data())) {
        return (std::string(buffer.data()));
    }

    return (std::nullopt);
}

uint32_t max_rx_pktlen(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::max_rx_pktlen));
}

uint32_t max_mac_addrs(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::max_mac_addrs));
}

uint32_t speeds(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::speed_capa));
}

uint32_t max_speed(uint16_t port_id)
{
    auto speed_capa = get_info_field(port_id, &rte_eth_dev_info::speed_capa);

    if (speed_capa & ETH_LINK_SPEED_100G) {
        return (ETH_SPEED_NUM_100G);
    } else if (speed_capa & ETH_LINK_SPEED_56G) {
        return (ETH_SPEED_NUM_56G);
    } else if (speed_capa & ETH_LINK_SPEED_50G) {
        return (ETH_SPEED_NUM_50G);
    } else if (speed_capa & ETH_LINK_SPEED_40G) {
        return (ETH_SPEED_NUM_40G);
    } else if (speed_capa & ETH_LINK_SPEED_25G) {
        return (ETH_SPEED_NUM_25G);
    } else if (speed_capa & ETH_LINK_SPEED_20G) {
        return (ETH_SPEED_NUM_20G);
    } else if (speed_capa & ETH_LINK_SPEED_10G) {
        return (ETH_SPEED_NUM_10G);
    } else if (speed_capa & ETH_LINK_SPEED_5G) {
        return (ETH_SPEED_NUM_5G);
    } else if (speed_capa & ETH_LINK_SPEED_2_5G) {
        return (ETH_SPEED_NUM_2_5G);
    } else if (speed_capa & ETH_LINK_SPEED_1G) {
        return (ETH_SPEED_NUM_1G);
    } else if (speed_capa & ETH_LINK_SPEED_100M
               || speed_capa & ETH_LINK_SPEED_100M_HD) {
        return (ETH_SPEED_NUM_100M);
    } else if (speed_capa & ETH_LINK_SPEED_10M
               || speed_capa & ETH_LINK_SPEED_10M_HD) {
        return (ETH_SPEED_NUM_10M);
    } else {
        return (ETH_SPEED_NUM_NONE);
    }
}

uint64_t rx_offloads(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::rx_offload_capa));
}

uint64_t tx_offloads(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::tx_offload_capa));
}

uint64_t rss_offloads(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::flow_type_rss_offloads));
}

uint16_t rx_queue_count(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::nb_rx_queues));
}

uint16_t rx_desc_count(uint16_t port_id)
{
    return std::min(
        (get_info_field(port_id, &rte_eth_dev_info::rx_desc_lim).nb_max),
        static_cast<uint16_t>(4096));
}

uint16_t tx_desc_count(uint16_t port_id)
{
    return std::min(
        (get_info_field(port_id, &rte_eth_dev_info::tx_desc_lim).nb_max),
        static_cast<uint16_t>(1024));
}

uint16_t tx_tso_segment_max(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::tx_desc_lim).nb_seg_max);
}

rte_eth_rxconf default_rxconf(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::default_rxconf));
}

rte_eth_txconf default_txconf(uint16_t port_id)
{
    return (get_info_field(port_id, &rte_eth_dev_info::default_txconf));
}

} // namespace openperf::packetio::dpdk::port_info
