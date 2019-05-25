#include "lwip/netif.h"

#include "packetio/stack/dpdk/netif_wrapper.h"

namespace icp {
namespace packetio {
namespace dpdk {

netif_wrapper::netif_wrapper(const std::string& id, const netif* ifp,
                             const interface::config_data& config)
    : m_id(id)
    , m_netif(ifp)
    , m_config(config)
{}

std::string netif_wrapper::id() const
{
    return (m_id);
}

int netif_wrapper::port_id() const
{
    return (m_config.port_id);
}

std::string netif_wrapper::mac_address() const
{
    return (net::to_string(net::mac_address(m_netif->hwaddr)));
}

std::string netif_wrapper::ipv4_address() const
{
    return (net::to_string(net::ipv4_address(ip_2_ip4(&m_netif->ip_addr)->addr)));
}

interface::stats_data netif_wrapper::stats() const
{
    return { .rx_packets = (m_netif->mib2_counters.ifinucastpkts
                            + m_netif->mib2_counters.ifinnucastpkts),
             .tx_packets = (m_netif->mib2_counters.ifoutucastpkts
                            + m_netif->mib2_counters.ifoutnucastpkts),
             .rx_bytes = m_netif->mib2_counters.ifinoctets,
             .tx_bytes = m_netif->mib2_counters.ifoutoctets,
             .rx_errors = (m_netif->mib2_counters.ifindiscards
                           + m_netif->mib2_counters.ifinerrors
                           + m_netif->mib2_counters.ifinunknownprotos),
             .tx_errors = (m_netif->mib2_counters.ifoutdiscards
                           + m_netif->mib2_counters.ifouterrors)
    };
}

interface::config_data netif_wrapper::config() const
{
    return (m_config);
}

}
}
}
