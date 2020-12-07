#include "arpa/inet.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/prot/dhcp.h"
#include "lwipopts.h"

#include "packet/stack/lwip/netif_utils.hpp"
#include "packet/stack/lwip/netif_wrapper.hpp"

namespace openperf::packet::stack {

netif_wrapper::netif_wrapper(std::string_view id,
                             const netif* ifp,
                             const packetio::interface::config_data& config)
    : m_id(id)
    , m_netif(ifp)
    , m_config(config)
{}

std::string netif_wrapper::id() const { return (m_id); }

std::string netif_wrapper::port_id() const { return (m_config.port_id); }

std::string netif_wrapper::mac_address() const
{
    return (libpacket::type::to_string(
        libpacket::type::mac_address(m_netif->hwaddr)));
}

packetio::interface::dhcp_client_state netif_wrapper::dhcp_state() const
{
    using dhcp_client_state = packetio::interface::dhcp_client_state;

    const auto* dhcp = netif_dhcp_data(m_netif);
    switch (dhcp ? dhcp->state : DHCP_STATE_OFF) {
    case DHCP_STATE_REQUESTING:
        return (dhcp_client_state::requesting);
    case DHCP_STATE_INIT:
        return (dhcp_client_state::init);
    case DHCP_STATE_REBOOTING:
        return (dhcp_client_state::rebooting);
    case DHCP_STATE_REBINDING:
        return (dhcp_client_state::rebinding);
    case DHCP_STATE_BACKING_OFF:
    case DHCP_STATE_SELECTING:
        return (dhcp_client_state::selecting);
    case DHCP_STATE_CHECKING:  /* stack verifies availability before using */
        return (dhcp_client_state::checking);
    case DHCP_STATE_BOUND:
        return (dhcp_client_state::bound);
    case DHCP_STATE_PERMANENT:  /* defined but not implemented by LwIP */
    case DHCP_STATE_RELEASING:  /* defined but not implemented by LwIP */
    case DHCP_STATE_INFORMING:
    case DHCP_STATE_OFF:
    default:
        return (dhcp_client_state::none);
    }
}

static libpacket::type::ipv4_address to_ipv4_address(const ip4_addr_t& addr)
{
    return (libpacket::type::ipv4_address(ntohl(addr.addr)));
}

std::optional<std::string> netif_wrapper::ipv4_address() const
{
    auto addr = ip_2_ip4(&m_netif->ip_addr);
    if (addr->addr) {
        return (libpacket::type::to_string(to_ipv4_address(*addr)));
    }
    return {};
}

std::optional<std::string> netif_wrapper::ipv4_gateway() const
{
    auto gw = ip_2_ip4(&m_netif->gw);
    if (gw->addr) {
        return (libpacket::type::to_string(to_ipv4_address(*gw)));
    }
    return {};
}

std::optional<uint8_t> netif_wrapper::ipv4_prefix_length() const
{
    auto netmask = ip_2_ip4(&m_netif->netmask);
    if (netmask->addr) {
        return (static_cast<uint8_t>(__builtin_popcount((*netmask).addr)));
    }
    return {};
}

static libpacket::type::ipv6_address to_ipv6_address(const ip6_addr_t& addr)
{
    return (libpacket::type::ipv6_address(
        reinterpret_cast<const uint8_t*>(addr.addr)));
}

std::optional<std::string> netif_wrapper::ipv6_address() const
{
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (ip6_addr_isinvalid(netif_ip6_addr_state(m_netif, i))) continue;
        auto addr = netif_ip6_addr(m_netif, i);
        if (ip6_addr_islinklocal(addr)) continue;
        return (libpacket::type::to_string(to_ipv6_address(*addr)));
    }
    return {};
}

std::optional<std::string> netif_wrapper::ipv6_linklocal_address() const
{
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (ip6_addr_isinvalid(netif_ip6_addr_state(m_netif, i))) continue;
        auto addr = netif_ip6_addr(m_netif, i);
        if (!ip6_addr_islinklocal(addr)) continue;
        return (libpacket::type::to_string(to_ipv6_address(*addr)));
    }
    return {};
}

std::any netif_wrapper::data() const { return (m_netif); }

packetio::interface::config_data netif_wrapper::config() const { return (m_config); }

packetio::interface::stats_data netif_wrapper::stats() const
{
    return {.rx_packets = (m_netif->mib2_counters.ifinucastpkts
                           + m_netif->mib2_counters.ifinnucastpkts),
            .tx_packets = (m_netif->mib2_counters.ifoutucastpkts
                           + m_netif->mib2_counters.ifoutnucastpkts),
            .rx_bytes = m_netif->mib2_counters.ifinoctets,
            .tx_bytes = m_netif->mib2_counters.ifoutoctets,
            .rx_errors = (m_netif->mib2_counters.ifindiscards
                          + m_netif->mib2_counters.ifinerrors
                          + m_netif->mib2_counters.ifinunknownprotos),
            .tx_errors = (m_netif->mib2_counters.ifoutdiscards
                          + m_netif->mib2_counters.ifouterrors)};
}

int netif_wrapper::input_packet(void* packet) const {
    return (netif_inject(const_cast<netif*>(m_netif), packet) == ERR_OK ? 0 : -1);
}

} // namespace openperf::packet::stack
