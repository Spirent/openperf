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
    return (net::to_string(net::mac_address(m_netif->hwaddr)));
}

std::string netif_wrapper::ipv4_address() const
{
    return (net::to_string(
        net::ipv4_address(ntohl(ip_2_ip4(&m_netif->ip_addr)->addr))));
}

std::string netif_wrapper::ipv6_address() const
{
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (ip6_addr_isinvalid(netif_ip6_addr_state(m_netif, i))) continue;
        auto addr = netif_ip6_addr(m_netif, i);
        if (ip6_addr_islinklocal(addr)) continue;
        return net::to_string(net::ipv6_address((const uint8_t*)addr->addr));
    }
    return net::to_string(net::ipv6_address());
}

std::string netif_wrapper::ipv6_linklocal_address() const
{
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
        if (ip6_addr_isinvalid(netif_ip6_addr_state(m_netif, i))) continue;
        auto addr = netif_ip6_addr(m_netif, i);
        if (!ip6_addr_islinklocal(addr)) continue;
        return net::to_string(net::ipv6_address((const uint8_t*)addr->addr));
    }
    return net::to_string(net::ipv6_address());
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
