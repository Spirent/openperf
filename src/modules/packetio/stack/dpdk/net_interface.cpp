#include <optional>
#include <variant>

#include "arpa/inet.h"

#include "lwip/netifapi.h"
#include "lwip/snmp.h"
#include "netif/ethernet.h"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/memp.h"
#include "packetio/memory/dpdk/pbuf.h"
#include "packetio/stack/dpdk/net_interface.h"

namespace icp {
namespace packetio {
namespace dpdk {

constexpr static char ifname_0 = 'i';
constexpr static char ifname_1 = 'o';

template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

/**
 * Retrieve the first instance of a protocol in the configuration vector.
 */
template <typename T>
static std::optional<T> get_protocol_config(const interface::config_data& config)
{
    for (auto& p : config.protocols) {
        if (std::holds_alternative<T>(p)) {
            return std::make_optional(std::get<T>(p));
        }
    }

    return std::nullopt;
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

static err_t net_interface_tx(netif* netif, pbuf *p)
{
    assert(netif);
    assert(p);

    net_interface *ifp = reinterpret_cast<net_interface*>(netif->state);

    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    if ((static_cast<uint8_t*>(p->payload)[0] & 1) == 0) {
        MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    } else {
        MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    }

    ICP_LOG(ICP_LOG_TRACE, "Transmitting packet from %c%c%u\n",
            netif->name[0], netif->name[1], netif->num);

    auto error = ifp->handle_tx(p);
    if (error != ERR_OK) {
        MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
    }
    return (error);
}

static err_t net_interface_rx(pbuf *p, netif* netif)
{
    assert(p);
    assert(netif);

    net_interface *ifp = reinterpret_cast<net_interface*>(netif->state);

    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if ((static_cast<uint8_t*>(p->payload)[0] & 1) == 0) {
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }

    ICP_LOG(ICP_LOG_TRACE, "Receiving packet for %c%c%u\n",
            netif->name[0], netif->name[1], netif->num);

    auto error = ifp->handle_rx(p);
    if (error != ERR_OK) {
        MIB2_STATS_NETIF_INC(netif, ifindiscards);
    }
    return (error);
}

static err_t net_interface_input(pbuf *p, netif* netif)
{
    assert(p);
    assert(netif);

    net_interface *ifp = reinterpret_cast<net_interface*>(netif->state);
    ifp->handle_input();
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
    auto eth_config = get_protocol_config<interface::eth_protocol_config>(config);
    for (size_t i = 0; i < ETH_HWADDR_LEN; i++) {
        netif->hwaddr[i] = eth_config->address[i];
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
        netif->flags |= NETIF_FLAG_LINK_UP;
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

static err_t setup_ipv4_interface(const std::optional<interface::ipv4_protocol_config> ipv4_config,
                                  netif& netif)
{
    err_t netif_error = ERR_OK;

    if (ipv4_config) {
        /* Explicit IPv4 config; so use it */
        std::visit(overloaded_visitor(
                       [&](const interface::ipv4_static_protocol_config& ipv4) {
                           ip4_addr address = { htonl(ipv4.address.data()) };
                           ip4_addr netmask = { htonl(to_netmask(ipv4.prefix_length)) };
                           ip4_addr gateway = { ipv4.gateway ? htonl(ipv4.gateway->data()) : 0 };
                           netif_error = (netifapi_netif_add(&netif,
                                                             &address, &netmask, &gateway,
                                                             netif.state,
                                                             net_interface_dpdk_init,
                                                             net_interface_rx)
                                          || netifapi_netif_set_up(&netif));
                       },
                       [&](const interface::ipv4_dhcp_protocol_config& dhcp) {
                           if (dhcp.hostname) {
                               netif_set_hostname(&netif, dhcp.hostname.value().c_str());
                           }
                           netif_error = (netifapi_netif_add(&netif,
                                                             nullptr, nullptr, nullptr,
                                                             netif.state,
                                                             net_interface_dpdk_init,
                                                             net_interface_rx)
                                          || netifapi_netif_set_up(&netif)
                                          || netifapi_dhcp_start(&netif));
                       }),
                   *ipv4_config);
    } else {
        /* No explicit IPv4 config; use AutoIP to pick a link-local IPv4 address */
        netif_error = (netifapi_netif_add(&netif,
                                          nullptr, nullptr, nullptr,
                                          netif.state, net_interface_dpdk_init, net_interface_rx)
                       || netifapi_netif_set_up(&netif)
                       || netifapi_autoip_start(&netif));
    }

    return (netif_error);
}

static void unset_ipv4_interface(const std::optional<interface::ipv4_protocol_config> ipv4,
                                 netif& netif)
{
    if (ipv4 && std::holds_alternative<interface::ipv4_dhcp_protocol_config>(*ipv4)) {
        netifapi_dhcp_release(&netif);
        netifapi_dhcp_stop(&netif);
    } else if (!ipv4) {
        netifapi_autoip_stop(&netif);
    }
}

static std::string recvq_name(int id)
{
    return (std::string("io") + std::to_string(id) + "_recvq");
}

net_interface::net_interface(int id, const interface::config_data& config)
    : m_config(config)
    , m_id(id)
    , m_recvq(rte_ring_create(recvq_name(id).c_str(), recvq_size, SOCKET_ID_ANY, RING_F_SC_DEQ))
{
    m_netif.state = this;

    /* Setup the stack interface */
    err_t netif_error = setup_ipv4_interface(get_protocol_config<interface::ipv4_protocol_config>(m_config),
                                             m_netif);
    if (netif_error) {
        throw std::runtime_error(lwip_strerr(netif_error));
    }

    /* Setup callbacks to allow the interface to interact with the port state */
    int dpdk_error = rte_eth_dev_callback_register(m_config.port_id,
                                                   RTE_ETH_EVENT_INTR_LSC,
                                                   net_interface_link_status_change,
                                                   &m_netif);
    if (dpdk_error) {
        throw std::runtime_error(rte_strerror(dpdk_error));
    }
}

net_interface::~net_interface()
{
    rte_eth_dev_callback_unregister(m_config.port_id,
                                    RTE_ETH_EVENT_INTR_LSC,
                                    net_interface_link_status_change,
                                    &m_netif);

    unset_ipv4_interface(get_protocol_config<interface::ipv4_protocol_config>(m_config),
                         m_netif);
    netifapi_netif_set_down(&m_netif);
    netifapi_netif_remove(&m_netif);
}

int net_interface::handle_rx(struct pbuf* p)
{
    auto empty = rte_ring_empty(m_recvq.get());
    if (rte_ring_enqueue(m_recvq.get(), p) != 0) {
        ICP_LOG(ICP_LOG_WARNING, "Receive queue full on %c%c%u\n",
                m_netif.name[0], m_netif.name[1], m_netif.num);
        return (ERR_BUF);
    }
    if (empty) tcpip_inpkt(p, &m_netif, net_interface_input);
    return (ERR_OK);
}

/**
 * Quick and dirty transmit function.
 * XXX: Assumes tx queue 0 exists.
 * TODO: Queue this to a dedicated tx thread/worker for bulk tx
 */
int net_interface::handle_tx(struct pbuf* p)
{
    /*
     * Bump the reference count on the mbuf before transmitting; when this
     * function returns, the stack will free the pbuf.  We don't want the
     * mbuf to be freed with it.
     */
    assert(p->next == NULL);
    auto mbuf = packetio_mbuf_synchronize(p);
    rte_mbuf_refcnt_update(mbuf, 1);

    rte_mbuf *pkts[] = { mbuf };
    return (rte_eth_tx_burst(port_id(), 0, pkts, 1) == 1 ? ERR_OK : ERR_BUF);
}

void net_interface::handle_input()
{
    static constexpr size_t rx_burst_size = 32;
    std::array<pbuf*, rx_burst_size> pbufs;

    do {
        auto nb_pbufs = rte_ring_dequeue_burst(m_recvq.get(),
                                               reinterpret_cast<void**>(pbufs.data()),
                                               rx_burst_size, nullptr);

        for (size_t i = 0; i < nb_pbufs; i++) {
            ethernet_input(pbufs[i], &m_netif);
        }
    } while (!rte_ring_empty(m_recvq.get()));
}

void* net_interface::operator new(size_t size)
{
    return (rte_zmalloc("net_interface", size, 0));
}

void net_interface::operator delete(void* ifp)
{
    rte_free(ifp);
}

int net_interface::id() const
{
    return (m_id);
}

int net_interface::port_id() const
{
    return (m_config.port_id);
}

netif* net_interface::data()
{
    return (&m_netif);
}

interface::config_data net_interface::config() const
{
    return (m_config);
}

}
}
}
