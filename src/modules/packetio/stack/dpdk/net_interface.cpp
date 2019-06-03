#include <optional>
#include <variant>

#include "arpa/inet.h"

#include "lwip/netifapi.h"
#include "lwip/snmp.h"
#include "netif/ethernet.h"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/memory/dpdk/memp.h"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/stack/dpdk/net_interface.h"
#include "packetio/stack/dpdk/offload_utils.h"

namespace icp {
namespace packetio {
namespace dpdk {

constexpr static char ifname_0 = 'i';
constexpr static char ifname_1 = 'o';

constexpr static uint16_t netif_rx_chksum_mask = 0xFF00;
constexpr static uint16_t netif_tx_chksum_mask = 0x00FF;

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

static uint16_t to_checksum_check_flags(uint64_t rx_offloads)
{
    /*
     * The netif flags control what the stack needs to check, so
     * start with all flags enabled and then disable what we can.
     */
    uint16_t flags = netif_rx_chksum_mask;

    if (rx_offloads & DEV_RX_OFFLOAD_IPV4_CKSUM) {
        flags &= ~NETIF_CHECKSUM_CHECK_IP;
    }

    if (rx_offloads & DEV_RX_OFFLOAD_UDP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_CHECK_UDP;
    }

    if (rx_offloads & DEV_RX_OFFLOAD_TCP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_CHECK_TCP;
    }

    return (flags);
}

static uint16_t to_checksum_gen_flags(uint64_t tx_offloads)
{
    uint16_t flags = netif_tx_chksum_mask;

    /*
     * The netif flags control what the stack needs to check, so
     * start with all flags enabled and then disable what we can.
     */
    if (tx_offloads & DEV_TX_OFFLOAD_IPV4_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_IP;
    }

    if (tx_offloads & DEV_TX_OFFLOAD_UDP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_UDP;
    }

    if (tx_offloads & DEV_TX_OFFLOAD_TCP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_TCP;
    }

    return (flags);
}

static uint32_t net_interface_max_gso_length(int port_id)
{
    auto info = model::port_info(port_id);
    return (info.tx_offloads() & DEV_TX_OFFLOAD_TCP_TSO
            ? info.tx_tso_segment_max() * TCP_MSS
            : TCP_MSS);
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

    /* Validate checksums; drop if invalid */
    auto mbuf = packetio_memory_pbuf_to_mbuf(p);
    if (mbuf->ol_flags & PKT_RX_IP_CKSUM_MASK
        && !(mbuf->ol_flags & PKT_RX_IP_CKSUM_GOOD)) {
        IP_STATS_INC(ip.chkerr);
        pbuf_free(p);
        return (ERR_OK);
    }

    if (mbuf->ol_flags & PKT_RX_L4_CKSUM_MASK
        && !(mbuf->ol_flags & PKT_RX_L4_CKSUM_GOOD)) {
        /* XXX: might not always be true? */
        assert(mbuf->packet_type);

        if (mbuf->packet_type & RTE_PTYPE_L4_TCP) {
            TCP_STATS_INC(tcp.chkerr);
        } else if (mbuf->packet_type & RTE_PTYPE_L4_UDP) {
            UDP_STATS_INC(udp.chkerr);
        } else {
            /* what is this?!?! */
            ICP_LOG(ICP_LOG_WARNING, "Unrecognized L4 packet type: %s\n",
                    rte_get_ptype_l4_name(mbuf->packet_type));
        }
        pbuf_free(p);
        return (ERR_OK);
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

    auto info = model::port_info(ifp->port_id());

    /* Initialize the snmp variables and counters in the netif struct */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, info.max_speed());

    netif->name[0] = ifname_0;
    netif->name[1] = ifname_1;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    auto config = ifp->config();
    auto eth_config = get_protocol_config<interface::eth_protocol_config>(config);
    for (size_t i = 0; i < ETH_HWADDR_LEN; i++) {
        netif->hwaddr[i] = eth_config->address[i];
    }

    rte_eth_dev_get_mtu(ifp->port_id(), &netif->mtu);

    netif->chksum_flags = (to_checksum_check_flags(info.rx_offloads())
                           | to_checksum_gen_flags(info.tx_offloads()));

    ICP_LOG(ICP_LOG_DEBUG, "Interface %c%c%u: mtu = %u, offloads = 0x%x\n",
            netif->name[0], netif->name[1], netif->num, netif->mtu, netif->chksum_flags);

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

static std::string recvq_name(const netif& netintf)
{
    return (std::string("io") + std::to_string(netintf.num) + "_recvq");
}

net_interface::net_interface(std::string_view id, const interface::config_data& config,
                             driver::tx_burst tx)
    : m_id(id)
    , m_max_gso_length(net_interface_max_gso_length(config.port_id))
    , m_config(config)
    , m_transmit(tx)
    , m_notify(false)
{
    m_netif.state = this;

    /* Setup the stack interface */
    err_t netif_error = setup_ipv4_interface(get_protocol_config<interface::ipv4_protocol_config>(m_config),
                                             m_netif);
    if (netif_error) {
        throw std::runtime_error(lwip_strerr(netif_error));
    }

    m_recvq = std::unique_ptr<rte_ring, rte_ring_deleter>
        ((rte_ring_create(recvq_name(m_netif).c_str(), recvq_size,
                          rte_eth_dev_socket_id(config.port_id), RING_F_SC_DEQ)));

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
    if (rte_ring_enqueue(m_recvq.get(), p) != 0) {
        ICP_LOG(ICP_LOG_WARNING, "Receive queue full on %c%c%u\n",
                m_netif.name[0], m_netif.name[1], m_netif.num);
        return (ERR_BUF);
    }
    if (!m_notify.test_and_set(std::memory_order_acquire)) {
        tcpip_inpkt(p, &m_netif, net_interface_input);
    }
    return (ERR_OK);
}

int net_interface::handle_tx(struct pbuf* p)
{
    /*
     * Bump the reference count on the mbuf chain before transmitting; when this
     * function returns, the stack will free the pbuf chain.  We don't want the
     * mbufs to be freed with it.
     */
    auto m_head = packetio_memory_mbuf_synchronize(p);
    for (auto m = m_head; m != nullptr; m = m->next) {
        rte_mbuf_refcnt_update(m, 1);
    }

    /* Setup tx offload metadata if offloads are enabled. */
    if (~m_netif.chksum_flags & netif_tx_chksum_mask) {
        set_tx_offload_metadata(m_head, m_netif.mtu);
    }

    rte_mbuf *pkts[] = { m_head };
    return (m_transmit(port_id(), 0, reinterpret_cast<void**>(pkts), 1) == 1 ? ERR_OK : ERR_BUF);
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

    m_notify.clear(std::memory_order_release);
}

void* net_interface::operator new(size_t size)
{
    return (rte_zmalloc("net_interface", size, 0));
}

void net_interface::operator delete(void* ifp)
{
    rte_free(ifp);
}

std::string net_interface::id() const
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

unsigned net_interface::max_gso_length() const
{
    return (m_max_gso_length);
}

interface::config_data net_interface::config() const
{
    return (m_config);
}

}
}
}
