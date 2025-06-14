#include <optional>
#include <variant>

#include "arpa/inet.h"

#include "rte_malloc.h"

#include "lwip/netifapi.h"
#include "lwip/snmp.h"
#if LWIP_IPV6
#include "lwip/ethip6.h"
#endif
#include "netif/ethernet.h"

#include "packet/stack/dpdk/offload_utils.hpp"
#include "packet/stack/dpdk/net_interface.hpp"
#include "packet/stack/dpdk/pbuf_utils.h"
// #include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_metadata.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#if LWIP_IPV6
#include "packet/stack/lwip/netifapi_ipv6.h"
#include "packet/stack/lwip/nd6_op.h"
#endif
#include "packet/stack/lwip/netifapi_utils.h"
#if LWIP_PACKET
#include "packet/stack/lwip/packet.h"
#endif
#include "utils/overloaded_visitor.hpp"

namespace openperf::packet::stack::dpdk {

using namespace openperf::packetio::dpdk;

constexpr static char ifname_0 = 'i';
constexpr static char ifname_1 = 'o';

constexpr static uint16_t netif_rx_chksum_mask = 0xFF00;
constexpr static uint16_t netif_tx_chksum_mask = 0x00FF;

/**
 * Retrieve the first instance of a protocol in the configuration vector.
 */
template <typename T>
const T* get_protocol_config(const packetio::interface::config_data& config)
{
    for (auto& p : config.protocols) {
        if (std::holds_alternative<T>(p)) {
            return std::addressof(std::get<T>(p));
        }
    }

    return nullptr;
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

    if (rx_offloads & RTE_ETH_RX_OFFLOAD_IPV4_CKSUM) {
        flags &= ~NETIF_CHECKSUM_CHECK_IP;
    }

    if (rx_offloads & RTE_ETH_RX_OFFLOAD_UDP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_CHECK_UDP;
    }

    if (rx_offloads & RTE_ETH_RX_OFFLOAD_TCP_CKSUM) {
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
    if (tx_offloads & RTE_ETH_TX_OFFLOAD_IPV4_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_IP;
    }

    if (tx_offloads & RTE_ETH_TX_OFFLOAD_UDP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_UDP;
    }

    if (tx_offloads & RTE_ETH_TX_OFFLOAD_TCP_CKSUM) {
        flags &= ~NETIF_CHECKSUM_GEN_TCP;
    }

    return (flags);
}

static uint32_t net_interface_max_gso_length(int port_id)
{
    return (port_info::tx_offloads(port_id) & RTE_ETH_TX_OFFLOAD_TCP_TSO
                ? port_info::tx_tso_segment_max(port_id) * TCP_MSS
                : TCP_MSS);
}

static err_t net_interface_tx(netif* netif, pbuf* p)
{
    assert(netif);
    assert(p);

    auto* ifp = reinterpret_cast<net_interface*>(netif->state);

    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    if ((static_cast<uint8_t*>(p->payload)[0] & 1) == 0) {
        MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    } else {
        MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
    }

    OP_LOG(OP_LOG_TRACE,
           "Transmitting packet from %c%c%u\n",
           netif->name[0],
           netif->name[1],
           netif->num);

    auto error = ifp->handle_tx(p);
    if (error != ERR_OK) {
        MIB2_STATS_NETIF_INC(netif, ifoutdiscards);
#ifdef LWIP_PACKET
    } else {
        packet_input(p, netif);
#endif
    }

    return (error);
}

static err_t net_interface_rx(pbuf* p, netif* netif)
{
    assert(p);
    assert(netif);

    /* Validate checksums; drop if invalid */
    auto* mbuf = packet_stack_pbuf_to_mbuf(p);
    if (mbuf->ol_flags & RTE_MBUF_F_RX_IP_CKSUM_MASK
        && !(mbuf->ol_flags & RTE_MBUF_F_RX_IP_CKSUM_GOOD)) {
        IP_STATS_INC(ip.chkerr);
        pbuf_free(p);
        return (ERR_OK);
    }

    if (mbuf->ol_flags & RTE_MBUF_F_RX_L4_CKSUM_MASK
        && !(mbuf->ol_flags & RTE_MBUF_F_RX_L4_CKSUM_GOOD)) {
        /* XXX: might not always be true? */
        assert(mbuf->packet_type);

        if (mbuf->packet_type & RTE_PTYPE_L4_TCP) {
            TCP_STATS_INC(tcp.chkerr);
        } else if (mbuf->packet_type & RTE_PTYPE_L4_UDP) {
            UDP_STATS_INC(udp.chkerr);
        } else {
            /* what is this?!?! */
            OP_LOG(OP_LOG_WARNING,
                   "Unrecognized L4 packet type: %s\n",
                   rte_get_ptype_l4_name(mbuf->packet_type));
        }
        pbuf_free(p);
        return (ERR_OK);
    }

    MIB2_STATS_NETIF_ADD_ATOMIC(netif, ifinoctets, p->tot_len);
    if ((static_cast<uint8_t*>(p->payload)[0] & 1) == 0) {
        MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
        MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }

    OP_LOG(OP_LOG_TRACE,
           "Receiving packet for %c%c%u\n",
           netif->name[0],
           netif->name[1],
           netif->num);

#ifdef LWIP_PACKET
    /* Distpatch to packet sockets */
    packet_input(p, netif);
#endif

    /* Then hand the packet off to the stack proper */
    return (ethernet_input(p, netif));
}

static err_t net_interface_dpdk_init(netif* netif)
{
    auto* ifp = reinterpret_cast<net_interface*>(netif->state);

    /* Initialize the snmp variables and counters in the netif struct */
    MIB2_INIT_NETIF(netif,
                    snmp_ifType_ethernet_csmacd,
                    port_info::max_speed(ifp->port_index()));

    netif->name[0] = ifname_0;
    netif->name[1] = ifname_1;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    auto config = ifp->config();
    const auto* eth_config =
        get_protocol_config<packetio::interface::eth_protocol_config>(config);
    for (size_t i = 0; i < ETH_HWADDR_LEN; i++) {
        netif->hwaddr[i] = eth_config->address[i];
    }

    rte_eth_dev_get_mtu(ifp->port_index(), &netif->mtu);

    netif->chksum_flags =
        (to_checksum_check_flags(port_info::rx_offloads(ifp->port_index()))
         | to_checksum_gen_flags(port_info::tx_offloads(ifp->port_index())));

    OP_LOG(OP_LOG_DEBUG,
           "Interface %c%c%u: mtu = %u, offloads = 0x%04hx\n",
           netif->name[0],
           netif->name[1],
           netif->num,
           netif->mtu,
           static_cast<uint16_t>(~netif->chksum_flags));

    netif->flags = (NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHERNET
                    | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP);

    netif->linkoutput = net_interface_tx;

    netif->output = etharp_output;

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#if LWIP_IPV6_MLD
    netif->flags |= NETIF_FLAG_MLD6;
#endif
#endif

    /* Finally, check link status and set UP flag if needed */
    rte_eth_link link;
    rte_eth_link_get_nowait(ifp->port_index(), &link);
    if (link.link_status == ETH_LINK_UP) { netif->flags |= NETIF_FLAG_LINK_UP; }

    return (ERR_OK);
}

static int net_interface_link_status_change(uint16_t port_id,
                                            enum rte_eth_event_type event,
                                            void* arg,
                                            void* ret_param
                                            __attribute__((unused)))
{
    if (event != RTE_ETH_EVENT_INTR_LSC) { return (0); }

    auto* netif = reinterpret_cast<struct netif*>(arg);
    rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);
    return (link.link_status == ETH_LINK_UP
                ? netifapi_netif_set_link_up(netif)
                : netifapi_netif_set_link_down(netif));
}

static err_t configure_ipv4_interface(
    const packetio::interface::ipv4_protocol_config* ipv4_config, netif& netif)
{
    err_t netif_error = ERR_OK;

    if (ipv4_config) {
        /* Explicit IPv4 config; so use it */
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv4_static_protocol_config&
                        ipv4) {
                    ip4_addr address = {htonl(ipv4.address.load<uint32_t>())};
                    ip4_addr netmask = {htonl(to_netmask(ipv4.prefix_length))};
                    ip4_addr gateway = {
                        ipv4.gateway ? htonl(ipv4.gateway->load<uint32_t>())
                                     : 0};
                    netif_error = netifapi_netif_set_addr(
                        &netif, &address, &netmask, &gateway);
                },
                [&](const packetio::interface::ipv4_auto_protocol_config&) {},
                [&](const packetio::interface::ipv4_dhcp_protocol_config&
                        dhcp) {
                    if (dhcp.hostname) {
                        netif_set_hostname(&netif,
                                           dhcp.hostname.value().c_str());
                    }
                }),
            *ipv4_config);
    }
    return (netif_error);
}

static err_t
start_ipv4_interface(const packetio::interface::ipv4_protocol_config* ipv4,
                     netif& netif)
{
    err_t netif_error = ERR_OK;

    if (ipv4) {
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv4_static_protocol_config&) {},
                [&](const packetio::interface::ipv4_auto_protocol_config&) {
                    netif_error = netifapi_autoip_start(&netif);
                },
                [&](const packetio::interface::ipv4_dhcp_protocol_config&) {
                    netif_error = netifapi_dhcp_start(&netif);
                }),
            *ipv4);
    }
    return (netif_error);
}

static void
stop_ipv4_interface(const packetio::interface::ipv4_protocol_config* ipv4,
                    netif& netif)
{
    if (ipv4) {
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv4_static_protocol_config&) {},
                [&](const packetio::interface::ipv4_auto_protocol_config&) {
                    netifapi_autoip_stop(&netif);
                },
                [&](const packetio::interface::ipv4_dhcp_protocol_config&) {
                    netifapi_dhcp_release(&netif);
                    netifapi_dhcp_stop(&netif);
                }),
            *ipv4);
    }
}

#if LWIP_IPV6

static err_t configure_ipv6_interface_link_local_address(
    netif& netif, const std::optional<libpacket::type::ipv6_address>& addr)
{
    if (addr) {
        struct ip6_addr ip6_addr;
        std::copy_n(addr->data(),
                    addr->size(),
                    reinterpret_cast<uint8_t*>(ip6_addr.addr));
        err_t err =
            netifapi_netif_add_ip6_address(&netif, &ip6_addr, 64, nullptr);
        return err;
    } else {
        /* Add auto link local IPv6 address */
        return netifapi_netif_create_ip6_linklocal_address(&netif, 1);
    }
}

static err_t configure_ipv6_interface_gateway_address(
    netif& netif, const std::optional<libpacket::type::ipv6_address>& addr)
{
    if (addr) {
        struct ip6_addr ip6_addr;
        std::copy_n(addr->data(),
                    addr->size(),
                    reinterpret_cast<uint8_t*>(ip6_addr.addr));
        ip6_addr.zone = IP6_NO_ZONE;
        return netifapi_netif_set_ip6_gateway(&netif, &ip6_addr);
    }
    return 0;
}

static err_t configure_ipv6_interface(
    const packetio::interface::ipv6_protocol_config* ipv6_config, netif& netif)
{
    err_t netif_error = ERR_OK;

    if (ipv6_config) {
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv6_static_protocol_config&
                        config) {
                    netif_error = configure_ipv6_interface_link_local_address(
                        netif, config.link_local_address);
                    if (netif_error != ERR_OK) return;

                    struct ip6_addr address;
                    std::copy_n(config.address.data(),
                                config.address.size(),
                                reinterpret_cast<uint8_t*>(address.addr));
                    netif_error = netifapi_netif_add_ip6_address(
                        &netif, &address, config.prefix_length, nullptr);
                    if (netif_error != ERR_OK) return;

                    netif_error = configure_ipv6_interface_gateway_address(
                        netif, config.gateway);
                    if (netif_error != ERR_OK) return;
                },
                [&](const packetio::interface::ipv6_auto_protocol_config&
                        config) {
                    netif_error = configure_ipv6_interface_link_local_address(
                        netif, config.link_local_address);
                    if (netif_error != ERR_OK) return;

                    netif_set_ip6_autoconfig_enabled(&netif, 1);
                },
                [&](const packetio::interface::ipv6_dhcp6_protocol_config&
                        config) {
                    netif_error = configure_ipv6_interface_link_local_address(
                        netif, config.link_local_address);
                    if (netif_error != ERR_OK) return;

                    if (config.stateless)
                        netif_set_ip6_autoconfig_enabled(&netif, 1);
                    else
                        netif_set_ip6_autoconfig_enabled(&netif, 0);
                }),
            *ipv6_config);
    }

    return (netif_error);
}

static err_t
start_ipv6_interface(const packetio::interface::ipv6_protocol_config* ipv6,
                     netif& netif)
{
    err_t netif_error = ERR_OK;

    if (ipv6) {
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv6_static_protocol_config&) {},
                [&](const packetio::interface::ipv6_auto_protocol_config&) {},
                [&](const packetio::interface::ipv6_dhcp6_protocol_config&
                        dhcp) {
                    if (dhcp.stateless)
                        netif_error = netifapi_dhcp6_enable_stateless(&netif);
                    else
                        netif_error = netifapi_dhcp6_enable_stateful(&netif);
                }),
            *ipv6);
    }
    return (netif_error);
}

static void
stop_ipv6_interface(const packetio::interface::ipv6_protocol_config* ipv6,
                    netif& netif)
{
    if (ipv6) {
        std::visit(
            utils::overloaded_visitor(
                [&](const packetio::interface::ipv6_static_protocol_config&) {},
                [&](const packetio::interface::ipv6_auto_protocol_config&) {},
                [&](const packetio::interface::ipv6_dhcp6_protocol_config&) {
                    netifapi_dhcp6_disable(&netif);
                }),
            *ipv6);
    }
}

#endif // LWIP_IPV6

net_interface::net_interface(std::string_view id,
                             int port_index,
                             const packetio::interface::config_data& config,
                             packetio::workers::transmit_function tx)
    : m_id(id)
    , m_port_index(port_index)
    , m_max_gso_length(net_interface_max_gso_length(port_index))
    , m_config(config)
    , m_transmit(tx)
    , m_rx_filter(config.rx_filter)
    , m_tx_filter(config.tx_filter)
{
    m_netif.state = this;

    /* Setup the stack interface */
    configure();
    try {
        up();
    } catch (...) {
        unconfigure();
        throw;
    }
}

net_interface::~net_interface()
{
    try {
        down();
        unconfigure();
    } catch (const std::exception& e) {
        OP_LOG(OP_LOG_ERROR,
               "Exception occured in net_interface destructor.   %s",
               e.what());
    }
}

void net_interface::configure()
{
    err_t netif_error = netifapi_netif_add(&m_netif,
                                           nullptr,
                                           nullptr,
                                           nullptr,
                                           m_netif.state,
                                           net_interface_dpdk_init,
                                           net_interface_rx);
    if (netif_error) { throw std::runtime_error(lwip_strerr(netif_error)); }

    netif_error = configure_ipv4_interface(
        get_protocol_config<packetio::interface::ipv4_protocol_config>(
            m_config),
        m_netif);
    if (netif_error) {
        netifapi_netif_remove(&m_netif);
        throw std::runtime_error(lwip_strerr(netif_error));
    }

#if LWIP_IPV6
    netif_error = configure_ipv6_interface(
        get_protocol_config<packetio::interface::ipv6_protocol_config>(
            m_config),
        m_netif);
    if (netif_error) {
        netifapi_netif_remove(&m_netif);
        throw std::runtime_error(lwip_strerr(netif_error));
    }
#endif // LWIP_IPV6
}

void net_interface::unconfigure()
{
    if (is_up()) down();

    rte_eth_dev_callback_unregister(m_port_index,
                                    RTE_ETH_EVENT_INTR_LSC,
                                    net_interface_link_status_change,
                                    &m_netif);

    netifapi_netif_remove(&m_netif);
}

bool net_interface::accept(const rte_mbuf* packet) const
{
    return (m_rx_filter.accept(packet));
}

bool net_interface::is_up() const
{
    return (m_netif.flags & NETIF_FLAG_UP) ? true : false;
}

void net_interface::up()
{
    if (is_up()) return;

    err_t netif_error = netifapi_netif_set_up(&m_netif);
    if (netif_error) { throw std::runtime_error(lwip_strerr(netif_error)); }

    netif_error = start_ipv4_interface(
        get_protocol_config<packetio::interface::ipv4_protocol_config>(
            m_config),
        m_netif);
    if (netif_error) {
        down();
        throw std::runtime_error(lwip_strerr(netif_error));
    }

#if LWIP_IPV6
    netif_error = start_ipv6_interface(
        get_protocol_config<packetio::interface::ipv6_protocol_config>(
            m_config),
        m_netif);
    if (netif_error) {
        down();
        throw std::runtime_error(lwip_strerr(netif_error));
    }
#endif // LWIP_IPV6
}

void net_interface::down()
{
    if (!is_up()) return;

    stop_ipv4_interface(
        get_protocol_config<packetio::interface::ipv4_protocol_config>(
            m_config),
        m_netif);

#if LWIP_IPV6
    stop_ipv6_interface(
        get_protocol_config<packetio::interface::ipv6_protocol_config>(
            m_config),
        m_netif);
#endif // LWIP_IPV6

    netifapi_netif_safe_set_down(&m_netif);
}

void net_interface::handle_link_state_change(bool link_up)
{
    OP_LOG(OP_LOG_INFO,
           "Interface %c%c%u Link %s\n",
           m_netif.name[0],
           m_netif.name[1],
           m_netif.num,
           link_up ? "Up" : "Down");
    link_up ? netifapi_netif_set_link_up(&m_netif)
            : netifapi_netif_set_link_down(&m_netif);
}

err_t net_interface::handle_tx(struct pbuf* p)
{
    /*
     * Bump the reference count on the mbuf chain before transmitting; when this
     * function returns, the stack will free the pbuf chain.  We don't want the
     * mbufs to be freed with it.
     */
    auto m_head = packet_stack_mbuf_synchronize(p);

    if (!m_head) { return ERR_BUF; }

    /* Store the interface hwaddr in the mbuf so tx capture can use it */
    mbuf_tx_sink_set(m_head, m_netif.hwaddr);

    /* Drop packets not matching the Tx filter. */
    if (!m_tx_filter.accept(m_head)) { return ERR_RTE; }

    for (auto m = m_head; m != nullptr; m = m->next) {
        rte_mbuf_refcnt_update(m, 1);
    }

    /* Setup tx offload metadata if offloads are enabled. */
    if (~m_netif.chksum_flags & netif_tx_chksum_mask) {
        set_tx_offload_metadata(m_head, m_netif.mtu);
    }

    rte_mbuf* pkts[] = {m_head};
    if (m_transmit(port_index(), 0, reinterpret_cast<void**>(pkts), 1) != 1) {
        return ERR_BUF;
    }

    return ERR_OK;
}

void* net_interface::operator new(size_t size)
{
    return (rte_zmalloc("net_interface", size, 0));
}

void net_interface::operator delete(void* ifp) { rte_free(ifp); }

std::string net_interface::id() const { return (m_id); }

std::string net_interface::port_id() const { return (m_config.port_id); }

int net_interface::port_index() const { return (m_port_index); }

const netif* net_interface::data() const { return (&m_netif); }

unsigned net_interface::max_gso_length() const { return (m_max_gso_length); }

packetio::interface::config_data net_interface::config() const
{
    return (m_config);
}

const net_interface& to_interface(netif* ifp)
{
    return *(reinterpret_cast<net_interface*>(ifp->state));
}

netif_ext& get_netif_ext(netif* ifp)
{
    auto& ni = *(reinterpret_cast<net_interface*>(ifp->state));
    return ni.m_netif_ext;
}

} // namespace openperf::packet::stack::dpdk

u8_t netif_ip6_prefix_len(netif* netif, int idx)
{
    auto& data = openperf::packet::stack::dpdk::get_netif_ext(netif);
    u8_t prefix_len = data.ip6_address_prefix_len[idx];
    /* Default prefix length is 64 */
    return prefix_len ? prefix_len : 64;
}

void netif_set_ip6_prefix_len(netif* netif, int idx, u8_t prefix_len)
{
    auto& data = openperf::packet::stack::dpdk::get_netif_ext(netif);
    data.ip6_address_prefix_len[idx] = prefix_len;
}

const ip6_addr_t* netif_ip6_gateway_addr(netif* netif)
{
    auto& data = openperf::packet::stack::dpdk::get_netif_ext(netif);
    if (!data.ip6_gateway) return nullptr;
    return &(*data.ip6_gateway);
}

void netif_set_ip6_gateway_addr(netif* netif, const ip6_addr_t* addr)
{
    auto& data = openperf::packet::stack::dpdk::get_netif_ext(netif);
    if (ip6_addr_isany(addr)) {
        data.ip6_gateway = {};
    } else {
        data.ip6_gateway = *addr;
        if (data.ip6_gateway->zone == IP6_NO_ZONE) {
            // Set gateway zone if required (i.e. link local address)
            ip6_addr_assign_zone(&*data.ip6_gateway, IP6_UNICAST, netif);
        }
    }
}
