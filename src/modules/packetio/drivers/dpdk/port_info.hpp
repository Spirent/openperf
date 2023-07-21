#ifndef _OP_PACKETIO_DPDK_PORT_INFO_HPP_
#define _OP_PACKETIO_DPDK_PORT_INFO_HPP_

#include <cstdint>
#include <optional>
#include <string>

#ifndef _NETINET_IP6_H
#define _NETINET_IP6_H
#endif
#include "rte_ethdev.h"

#include "lib/packet/type/mac_address.hpp"

namespace openperf::packetio::dpdk::port_info {

unsigned socket_id(uint16_t port_id);

std::string driver_name(uint16_t port_id);
std::string device_name(uint16_t port_id);
std::optional<std::string> interface_name(uint16_t port_id);
libpacket::type::mac_address mac_address(uint16_t port_id);

uint32_t speeds(uint16_t port_id);
uint32_t max_speed(uint16_t port_id);
uint32_t max_rx_pktlen(uint16_t port_id);
uint32_t max_mtu(uint16_t port_id);
uint32_t max_lro_pkt_size(uint16_t port_id);
uint32_t max_mac_addrs(uint16_t port_id);

uint64_t rx_offloads(uint16_t port_id);
uint64_t tx_offloads(uint16_t port_id);
uint64_t rss_offloads(uint16_t port_id);

enum rte_eth_rx_mq_mode rx_mq_mode(uint16_t port_id);

uint16_t rx_queue_count(uint16_t port_id);
uint16_t rx_queue_max(uint16_t port_id);
uint16_t rx_queue_default(uint16_t port_id);

uint16_t tx_queue_count(uint16_t port_id);
uint16_t tx_queue_max(uint16_t port_id);
uint16_t tx_queue_default(uint16_t port_id);

uint16_t rx_desc_count(uint16_t port_id);
uint16_t tx_desc_count(uint16_t port_id);

rte_eth_rxconf default_rxconf(uint16_t port_id);
rte_eth_txconf default_txconf(uint16_t port_id);

uint16_t tx_tso_segment_max(uint16_t port_id);

bool lsc_interrupt(uint16_t port_id);

template <typename T>
static T get_info_field(int id, T rte_eth_dev_info::*field)
{
    auto info = rte_eth_dev_info{};
    rte_eth_dev_info_get(id, &info);
    return (info.*field);
}

} // namespace openperf::packetio::dpdk::port_info

#endif /* _OP_PACKETIO_DPDK_PORT_INFO_HPP */
