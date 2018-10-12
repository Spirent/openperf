#include "packetio/generic_port.h"
#include "drivers/dpdk/dpdk.h"
#include "drivers/dpdk/model/physical_port.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace model {

physical_port::physical_port(int id)
    : m_id(id)
{}

int physical_port::id() const
{
    return m_id;
}

std::string physical_port::kind()
{
    return "dpdk";
}

port::link_status physical_port::link() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_id, &link);
    return (link.link_status == ETH_LINK_UP
            ? port::link_status::LINK_UP
            : port::link_status::LINK_DOWN);
}

port::link_speed physical_port::speed() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_id, &link);
    return (link.link_status == ETH_LINK_UP
            ? static_cast<port::link_speed>(link.link_speed)
            : port::link_speed::SPEED_UNKNOWN);
}

port::link_duplex physical_port::duplex() const
{
    struct rte_eth_link link;
    rte_eth_link_get_nowait(m_id, &link);
    return (link.link_duplex == ETH_LINK_FULL_DUPLEX
            ? port::link_duplex::DUPLEX_FULL
            : port::link_duplex::DUPLEX_HALF);
}

port::stats_data physical_port::stats() const
{
    struct rte_eth_stats stats;
    rte_eth_stats_get(m_id, &stats);

    return {
        .rx_packets = static_cast<int64_t>(stats.ipackets),
        .tx_packets = static_cast<int64_t>(stats.opackets),
        .rx_bytes = static_cast<int64_t>(stats.ibytes),
        .tx_bytes = static_cast<int64_t>(stats.obytes),
        .rx_errors = static_cast<int64_t>(stats.imissed + stats.ierrors + stats.rx_nombuf),
        .tx_errors = static_cast<int64_t>(stats.oerrors)
    };
}

port::config_data physical_port::config() const
{
    // TODO
    return port::config_data(port::dpdk_config{.auto_negotiation = true});
}

tl::expected<void, std::string> physical_port::config(const port::config_data& c)
{
    // TODO
    (void)c;
    return {};
}

}
}
}
}
