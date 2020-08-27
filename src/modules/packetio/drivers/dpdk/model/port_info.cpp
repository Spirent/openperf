#include <algorithm>
#include <limits>

#include <net/if.h>

#include "core/op_core.h"
#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"

namespace openperf::packetio::dpdk::model {

template <typename T>
static T get_info_field(int id, T rte_eth_dev_info::*field)
{
    rte_eth_dev_info info;
    rte_eth_dev_info_get(id, &info);
    return (info.*field);
}

static uint16_t operator"" _S(unsigned long long value)
{
    return static_cast<uint16_t>(value);
}

port_info::port_info(uint16_t port_id)
    : m_id(port_id)
{
    if (!rte_eth_dev_is_valid_port(m_id)) {
        throw std::runtime_error("Port id " + std::to_string(m_id)
                                 + " is invalid");
    }
}

uint16_t port_info::id() const { return (m_id); }

unsigned port_info::socket_id() const
{
    auto id = rte_eth_dev_socket_id(m_id);
    return (id >= 0 ? id : 0);
}

std::string port_info::driver_name() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::driver_name));
}

std::string port_info::device_name() const
{
    auto buffer = std::array<char, RTE_ETH_NAME_MAX_LEN>{};
    if (rte_eth_dev_get_name_by_port(m_id, buffer.data()) == 0) {
        return (std::string(buffer.data()));
    }

    return (std::string{});
}

std::optional<std::string> port_info::interface_name() const
{
    auto if_index = get_info_field(m_id, &rte_eth_dev_info::if_index);
    auto buffer = std::array<char, IF_NAMESIZE>{};
    if (if_indextoname(if_index, buffer.data())) {
        return (std::string(buffer.data()));
    }

    return (std::nullopt);
}

uint32_t port_info::max_rx_pktlen() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::max_rx_pktlen));
}

uint32_t port_info::max_mac_addrs() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::max_mac_addrs));
}

uint32_t port_info::speeds() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::speed_capa));
}

uint32_t port_info::max_speed() const
{
    auto speed_capa = get_info_field(m_id, &rte_eth_dev_info::speed_capa);

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

uint64_t port_info::rx_offloads() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::rx_offload_capa));
}

uint64_t port_info::tx_offloads() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::tx_offload_capa));
}

uint64_t port_info::rss_offloads() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::flow_type_rss_offloads));
}

uint16_t port_info::rx_queue_count() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::nb_rx_queues));
}

uint16_t port_info::rx_queue_max() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::max_rx_queues));
}

uint16_t port_info::rx_queue_default() const
{
    return (std::max(
        1_S,
        get_info_field(m_id, &rte_eth_dev_info::default_rxportconf).nb_queues));
}

uint16_t port_info::tx_queue_count() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::nb_tx_queues));
}

uint16_t port_info::tx_queue_max() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::max_tx_queues));
}

uint16_t port_info::tx_queue_default() const
{
    return (std::max(
        1_S,
        get_info_field(m_id, &rte_eth_dev_info::default_txportconf).nb_queues));
}

uint16_t port_info::rx_desc_count() const
{
    return std::min(
        (get_info_field(m_id, &rte_eth_dev_info::rx_desc_lim).nb_max),
        static_cast<uint16_t>(4096));
}

uint16_t port_info::tx_desc_count() const
{
    return std::min(
        (get_info_field(m_id, &rte_eth_dev_info::tx_desc_lim).nb_max),
        static_cast<uint16_t>(1024));
}

uint16_t port_info::tx_tso_segment_max() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::tx_desc_lim).nb_seg_max);
}

rte_eth_rxconf port_info::default_rxconf() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::default_rxconf));
}

rte_eth_txconf port_info::default_txconf() const
{
    return (get_info_field(m_id, &rte_eth_dev_info::default_txconf));
}

bool port_info::lsc_interrupt() const
{
    return (*(get_info_field(m_id, &rte_eth_dev_info::dev_flags))
            & RTE_ETH_DEV_INTR_LSC);
}

bool port_info::rxq_interrupt() const
{
    /*
     * There seems to be no programatic way to determine whether a device
     * supports rx queue interrupts or not, so we attempt to enable them on
     * everything, unless the user says otherwise.  Run-time errors will disable
     * them.
     */
    static auto result =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
            "modules.packetio.dpdk.no-rx-interrupts");
    /* XXX: A negative times a negative equals a positive. Say it! */
    return (!result.value_or(false));
}

} // namespace openperf::packetio::dpdk::model
