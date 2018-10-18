#include <optional>
#include <variant>

#include "lwip/netifapi.h"
#include "lwip/snmp.h"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/net_interface.h"

namespace icp {
namespace packetio {
namespace dpdk {

constexpr static char ifname_0 = 'i';
constexpr static char ifname_1 = 'o';

static std::optional<interface::ipv4_protocol_config>
get_ipv4_protocol_config(interface::config_data& config)
{
    for (auto& p : config.protocols) {
        if (std::holds_alternative<interface::ipv4_protocol_config>(p)) {
            return std::make_optional(std::get<interface::ipv4_protocol_config>(p));
        }
    }
    return std::nullopt;
}

static interface::eth_protocol_config
get_eth_protocol_config(interface::config_data& config)
{
    for (auto& p : config.protocols) {
        if (std::holds_alternative<interface::eth_protocol_config>(p)) {
            return std::get<interface::eth_protocol_config>(p);
        }
    }

    throw std::runtime_error("No Ethernet protocol configuration found!");
}

static bool is_dhcp(interface::config_data& config)
{
    auto ipv4_config = get_ipv4_protocol_config(config);
    if (ipv4_config
        && std::holds_alternative<interface::ipv4_dhcp_protocol_config>(*ipv4_config)) {
        return true;
    }
    return false;
}

static bool has_ipv4(interface::config_data& config)
{
    for (auto& p : config.protocols) {
        if (std::holds_alternative<interface::ipv4_protocol_config>(p)) {
            return true;
        }
    }
    return false;
}

static bool has_static_ipv4(interface::config_data& config)
{
    return (has_ipv4(config) && !is_dhcp(config));
}

static uint32_t to_netmask(int prefix_length)
{
    return (static_cast<uint32_t>(~0) << (32 - prefix_length));
}

static uint32_t dpdk_port_speed(uint16_t port_id)
{
    /* If the port has link, return the current speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);
    if (link.link_status == ETH_LINK_UP) {
        return (link.link_speed);
    }

    /* If there is no link, see if the port knows about it's speeds */
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_id, &info);

    if (info.speed_capa & ETH_LINK_SPEED_100G) {
        return (ETH_SPEED_NUM_100G);
    } else if (info.speed_capa & ETH_LINK_SPEED_56G) {
        return (ETH_SPEED_NUM_56G);
    } else if (info.speed_capa & ETH_LINK_SPEED_50G) {
        return (ETH_SPEED_NUM_50G);
    } else if (info.speed_capa & ETH_LINK_SPEED_40G) {
        return (ETH_SPEED_NUM_40G);
    } else if (info.speed_capa & ETH_LINK_SPEED_25G) {
        return (ETH_SPEED_NUM_25G);
    } else if (info.speed_capa & ETH_LINK_SPEED_20G) {
        return (ETH_SPEED_NUM_20G);
    } else if (info.speed_capa & ETH_LINK_SPEED_10G) {
        return (ETH_SPEED_NUM_10G);
    } else if (info.speed_capa & ETH_LINK_SPEED_5G) {
        return (ETH_SPEED_NUM_5G);
    } else if (info.speed_capa & ETH_LINK_SPEED_2_5G) {
        return (ETH_SPEED_NUM_2_5G);
    } else if (info.speed_capa & ETH_LINK_SPEED_1G) {
        return (ETH_SPEED_NUM_1G);
    } else if (info.speed_capa & ETH_LINK_SPEED_100M
               || info.speed_capa & ETH_LINK_SPEED_100M_HD) {
        return (ETH_SPEED_NUM_100M);
    } else if (info.speed_capa & ETH_LINK_SPEED_10M
               || info.speed_capa & ETH_LINK_SPEED_10M_HD) {
        return (ETH_SPEED_NUM_10M);
    } else {
        /*
         * If we don't have any speed capabilities, return the link speed.
         * It might be hard coded.
         */
        return (link.link_speed ? link.link_speed : ETH_SPEED_NUM_NONE);
    }
}

/* TODO */
static err_t net_interface_tx(netif* netif, pbuf *p)
{
    if (!p) {
        return (ERR_OK);
    }

    icp_log(ICP_LOG_INFO, "Tx called for pbuf %p on netif %p\n",
            (void *)p, (void *)netif);
    return (ERR_OK);
}

static err_t net_interface_dpdk_init(netif* netif)
{
    net_interface *ifp = reinterpret_cast<net_interface*>(netif->state);

    /* Initialize the snmp variables and counters in the netif struct */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, dpdk_port_speed(ifp->port_id()));

    netif->name[0] = ifname_0;
    netif->name[1] = ifname_1;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    auto config = ifp->config();
    auto eth_config = get_eth_protocol_config(config);
    for (size_t i = 0; i < ETH_HWADDR_LEN; i++) {
        netif->hwaddr[i] = eth_config.address[i];
    }

    uint16_t mtu;
    rte_eth_dev_get_mtu(ifp->port_id(), &mtu);
    netif->mtu = mtu;

    netif->flags = (NETIF_FLAG_BROADCAST
                    | NETIF_FLAG_ETHERNET
                    | NETIF_FLAG_ETHARP
                    | NETIF_FLAG_IGMP);

    netif->linkoutput = net_interface_tx;

    netif->output = etharp_output;

#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif->flags |= NETIF_FLAG_MLD6;
    /* TODO: IPv6 setup */
#endif

    /* Finally, check link status and set UP flag if needed */
    rte_eth_link link;
    rte_eth_link_get_nowait(ifp->port_id(), &link);
    if (link.link_status == ETH_LINK_UP) {
        netif->flags |= NETIF_FLAG_UP;
    }

    return (ERR_OK);
}

static int net_interface_link_status_change(uint16_t port_id,
                                            enum rte_eth_event_type event,
                                            void *arg,
                                            void *ret_param __attribute__((unused)))
{
    if (event != RTE_ETH_EVENT_INTR_LSC) {
        return (0);
    }

    netif* netif = reinterpret_cast<struct netif*>(arg);
    rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);
    return (link.link_status == ETH_LINK_UP
            ? netifapi_netif_set_link_up(netif)
            : netifapi_netif_set_link_down(netif));
}

net_interface::net_interface(int id, int port_id, const interface::config_data& config)
    : m_config(config)
    , m_portid(port_id)
    , m_id(id)
{
    m_netif.state = this;

    /* Setup the stack interface */
    err_t netif_error;
    if (has_ipv4(m_config)) {
        if (has_static_ipv4(m_config)) {
            auto ipv4_config = std::get<interface::ipv4_static_protocol_config>(
                *get_ipv4_protocol_config(m_config));
            ip4_addr address = { ipv4_config.address.data() };
            ip4_addr netmask = { to_netmask(ipv4_config.prefix_length) };
            ip4_addr gateway = { ipv4_config.gateway ? ipv4_config.gateway->data() : 0 };
            netif_error = (netifapi_netif_add(&m_netif,
                                              &address, &netmask, &gateway,
                                              this, net_interface_dpdk_init, ip_input)
                           || netifapi_netif_set_up(&m_netif));
        } else {
            /* DHCP controlled interface */
            auto dhcp_config = std::get<interface::ipv4_dhcp_protocol_config>(
                *get_ipv4_protocol_config(m_config));
            if (dhcp_config.hostname) {
                netif_set_hostname(&m_netif, dhcp_config.hostname.value().c_str());
            }
            netif_error = (netifapi_netif_add(&m_netif,
                                              nullptr, nullptr, nullptr,
                                              this, net_interface_dpdk_init, ip_input)
                           || netifapi_netif_set_up(&m_netif)
                           || netifapi_dhcp_start(&m_netif));
        }
    } else {
        netif_error = (netifapi_netif_add(&m_netif,
                                          nullptr, nullptr, nullptr,
                                          this, net_interface_dpdk_init, ip_input)
                       || netifapi_netif_set_up(&m_netif)
                       || netifapi_autoip_start(&m_netif));
    }

    if (netif_error) {
        throw std::runtime_error(lwip_strerr(netif_error));
    }

    /* Setup callbacks to allow the interface to interact with the port state */
    int dpdk_error = rte_eth_dev_callback_register(m_portid,
                                                   RTE_ETH_EVENT_INTR_LSC,
                                                   net_interface_link_status_change,
                                                   &m_netif);
    if (dpdk_error) {
        throw std::runtime_error(rte_strerror(dpdk_error));
    }
}

net_interface::~net_interface()
{
    rte_eth_dev_callback_unregister(m_portid,
                                    RTE_ETH_EVENT_INTR_LSC,
                                    net_interface_link_status_change,
                                    &m_netif);

    if (has_ipv4(m_config)) {
        if (is_dhcp(m_config)) {
            netifapi_dhcp_release(&m_netif);
            netifapi_dhcp_stop(&m_netif);
        }
    } else {
        netifapi_autoip_stop(&m_netif);
    }

    netifapi_netif_set_down(&m_netif);
    netifapi_netif_remove(&m_netif);
}

int net_interface::id() const
{
    return (m_id);
}

int net_interface::port_id() const
{
    return (m_portid);
}

std::string net_interface::mac_address() const
{
    return (net::to_string(net::mac_address(m_netif.hwaddr)));
}

std::string net_interface::ipv4_address() const
{
    return (net::to_string(net::ipv4_address(m_netif.ip_addr.addr)));
}

interface::stats_data net_interface::stats() const
{
    return { .rx_packets = (m_netif.mib2_counters.ifinucastpkts
                            + m_netif.mib2_counters.ifinnucastpkts),
             .tx_packets = (m_netif.mib2_counters.ifoutucastpkts
                            + m_netif.mib2_counters.ifoutnucastpkts),
             .rx_bytes = m_netif.mib2_counters.ifinoctets,
             .tx_bytes = m_netif.mib2_counters.ifoutoctets,
             .rx_errors = (m_netif.mib2_counters.ifindiscards
                           + m_netif.mib2_counters.ifinerrors
                           + m_netif.mib2_counters.ifinunknownprotos),
             .tx_errors = (m_netif.mib2_counters.ifoutdiscards
                           + m_netif.mib2_counters.ifouterrors)
    };
}

interface::config_data net_interface::config() const
{
    return (m_config);
}

}
}
}
