#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>

#include <net/if.h>

#include "core/icp_core.h"
#include "swagger/v1/model/Port.h"
#include "packetio/port_api.h"
#include "drivers/dpdk/dpdk.h"

using namespace swagger::v1::model;

namespace icp {
namespace packetio {
namespace port {
namespace api {
namespace impl {

static std::map<int, std::string> tx_offloads = {
    { DEV_TX_OFFLOAD_VLAN_INSERT,      "VLAN insertion" },
    { DEV_TX_OFFLOAD_IPV4_CKSUM,       "IPv4 checksums" },
    { DEV_TX_OFFLOAD_UDP_CKSUM,        "UDP checksums" },
    { DEV_TX_OFFLOAD_TCP_CKSUM,        "TCP checksums" },
    { DEV_TX_OFFLOAD_SCTP_CKSUM,       "SCTP checksums" },
    { DEV_TX_OFFLOAD_TCP_TSO,          "TCP TSO" },
    { DEV_TX_OFFLOAD_UDP_TSO,          "UDP TSO" },
    { DEV_TX_OFFLOAD_OUTER_IPV4_CKSUM, "Outer IPv4 checksums" },
    { DEV_TX_OFFLOAD_QINQ_INSERT,      "QinQ insertion" },
    { DEV_TX_OFFLOAD_VXLAN_TNL_TSO,    "VxLAN tunneling TSO" },
    { DEV_TX_OFFLOAD_GRE_TNL_TSO,      "GRE tunneling TSO" },
    { DEV_TX_OFFLOAD_IPIP_TNL_TSO,     "IP over IP tunneling TSO" },
    { DEV_TX_OFFLOAD_GENEVE_TNL_TSO,   "GENEVE tunneling TSO" },
    { DEV_TX_OFFLOAD_MACSEC_INSERT,    "MACSec insertion" },
};

static std::map<int, std::string> rx_offloads = {
    { DEV_RX_OFFLOAD_VLAN_STRIP,       "VLAN removal" },
    { DEV_RX_OFFLOAD_IPV4_CKSUM,       "IPv4 checksums" },
    { DEV_RX_OFFLOAD_UDP_CKSUM,        "UDP checksums" },
    { DEV_RX_OFFLOAD_TCP_CKSUM,        "TCP checksums" },
    { DEV_RX_OFFLOAD_TCP_LRO,          "TCP Large Receive Offloads" },
    { DEV_RX_OFFLOAD_QINQ_STRIP,       "QinQ removal" },
    { DEV_RX_OFFLOAD_OUTER_IPV4_CKSUM, "Outer IPv4 checksums" },
    { DEV_RX_OFFLOAD_MACSEC_STRIP,     "MACSec removal" },
    { DEV_RX_OFFLOAD_HEADER_SPLIT,     "Header splitting" },
    { DEV_RX_OFFLOAD_VLAN_FILTER,      "VLAN filtering" },
    { DEV_RX_OFFLOAD_VLAN_EXTEND,      "VLAN extending" },
    { DEV_RX_OFFLOAD_JUMBO_FRAME,      "Jumbo frames" },
    { DEV_RX_OFFLOAD_CRC_STRIP,        "CRC removal" },
    { DEV_RX_OFFLOAD_SCATTER,          "Rx scatter" },
    { DEV_RX_OFFLOAD_TIMESTAMP,        "Timestamping" },
    { DEV_RX_OFFLOAD_SECURITY,         "Security" }
};

bool is_valid_port(int port_idx)
{
    return (rte_eth_dev_is_valid_port(port_idx) == 1);
}

void list_ports(std::vector<std::shared_ptr<Port>> &ports, std::string &kind)
{
    uint16_t port_idx = 0;
    RTE_ETH_FOREACH_DEV(port_idx) {
        if (!kind.length() | kind == "dpdk") {
            ports.push_back(get_port(port_idx));
        }
    }
}

static std::shared_ptr<PortDriver> get_port_driver(int id)
{
    assert(rte_eth_dev_is_valid_port(id));

    auto dpdk_driver = std::make_shared<PortDriver_dpdk>();

    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(id, &info);

    dpdk_driver->setDriverName(info.driver_name);

    // TODO: pci address

    dpdk_driver->setMinRxBufferSize(info.min_rx_bufsize);
    dpdk_driver->setMaxRxPacketLength(info.max_rx_pktlen);
    dpdk_driver->setMaxRxQueues(info.max_rx_queues);
    dpdk_driver->setMaxTxQueues(info.max_tx_queues);
    dpdk_driver->setMaxMacAddresses(info.max_mac_addrs);

    for (auto &pair : rx_offloads) {
        if (info.rx_offload_capa & pair.first) {
            dpdk_driver->getRxOffloads().push_back(pair.second);
        }
    }

    for (auto &pair : tx_offloads) {
        if (info.tx_offload_capa & pair.first) {
            dpdk_driver->getTxOffloads().push_back(pair.second);
        }
    }

    auto driver = std::make_shared<PortDriver>();
    driver->setDpdk(dpdk_driver);

    return (driver);
}

static std::shared_ptr<PortConfig> get_port_config(int id)
{
    assert(rte_eth_dev_is_valid_port(id));

    // XXX: Only dpdk for now
    auto dpdk_config = std::make_shared<PortConfig_dpdk>();
    dpdk_config->setAutoNegotiation(true);
    dpdk_config->unsetSpeed();
    dpdk_config->unsetDuplex();

    auto config = std::make_shared<PortConfig>();
    config->setDpdk(dpdk_config);

    return (config);
}

static std::shared_ptr<PortStatus> get_port_status(int id)
{
    assert(rte_eth_dev_is_valid_port(id));

    auto status = std::make_shared<PortStatus>();

    struct rte_eth_link link;
    rte_eth_link_get_nowait(id, &link);

    status->setLink(link.link_status == ETH_LINK_DOWN ? "down" : "up");
    status->setSpeed(link.link_speed);
    status->setDuplex(link.link_duplex == ETH_LINK_HALF_DUPLEX ? "half" : "full");

    return (status);
}

static std::shared_ptr<PortStats> get_port_stats(int id)
{
    assert(rte_eth_dev_is_valid_port(id));

    auto stats = std::make_shared<PortStats>();

    struct rte_eth_stats port_stats;
    rte_eth_stats_get(id, &port_stats);

    stats->setRxPackets(port_stats.ipackets);
    stats->setTxPackets(port_stats.opackets);
    stats->setRxBytes(port_stats.ibytes);
    stats->setTxBytes(port_stats.obytes);
    stats->setRxErrors(port_stats.imissed + port_stats.ierrors);
    stats->setTxErrors(port_stats.oerrors);

    return (stats);
}

std::shared_ptr<Port> get_port(int id)
{
    assert(rte_eth_dev_is_valid_port(id));

    auto port = std::make_shared<Port>();

    port->setId(std::to_string(id));
    port->setKind("dpdk");
    port->setDriver(get_port_driver(id));
    port->setConfig(get_port_config(id));
    port->setStatus(get_port_status(id));
    port->setStats(get_port_stats(id));

    return (port);
}

int update_port(int id, std::shared_ptr<Port> port)
{

    return (0);
}

void delete_port(int id)
{

}

}
}
}
}
}
