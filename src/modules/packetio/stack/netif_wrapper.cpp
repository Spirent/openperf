#include "arpa/inet.h"
#include "lwip/netif.h"

#include "packetio/stack/netif_wrapper.hpp"

namespace openperf::packetio {

netif_wrapper::netif_wrapper(std::string_view id,
                             const netif* ifp,
                             const interface::config_data& config)
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

interface::config_data netif_wrapper::config() const { return (m_config); }

interface::stats_data netif_wrapper::stats() const
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

} // namespace openperf::packetio
