#ifndef _OP_PACKETIO_DPDK_PHYSICAL_PORT_TCC_
#define _OP_PACKETIO_DPDK_PHYSICAL_PORT_TCC_

#include <string>

#include "packetio/generic_port.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/port_info.hpp"

namespace openperf::packetio::dpdk {

template <typename Derived> class physical_port
{
public:
    std::string id() const { return (m_id); }

    static std::string kind() { return "dpdk"; }

    port::link_speed speed() const
    {
        auto link = rte_eth_link{};
        rte_eth_link_get_nowait(m_idx, &link);
        return (link.link_status == ETH_LINK_UP
                    ? static_cast<port::link_speed>(link.link_speed)
                    : port::link_speed::SPEED_UNKNOWN);
    }

    port::link_status link() const
    {
        auto link = rte_eth_link{};
        rte_eth_link_get_nowait(m_idx, &link);
        return (link.link_status == ETH_LINK_UP ? port::link_status::LINK_UP
                                                : port::link_status::LINK_DOWN);
    }

    port::link_duplex duplex() const
    {
        auto link = rte_eth_link{};
        rte_eth_link_get_nowait(m_idx, &link);
        return (link.link_duplex == ETH_LINK_FULL_DUPLEX
                    ? port::link_duplex::DUPLEX_FULL
                    : port::link_duplex::DUPLEX_HALF);
    }

    port::stats_data stats() const
    {
        auto stats = rte_eth_stats{};
        rte_eth_stats_get(m_idx, &stats);

        return {.rx_packets = static_cast<int64_t>(stats.ipackets),
                .tx_packets = static_cast<int64_t>(stats.opackets),
                .rx_bytes = static_cast<int64_t>(stats.ibytes),
                .tx_bytes = static_cast<int64_t>(stats.obytes),
                .rx_errors = static_cast<int64_t>(stats.imissed + stats.ierrors
                                                  + stats.rx_nombuf),
                .tx_errors = static_cast<int64_t>(stats.oerrors)};
    }

    port::config_data config() const
    {
        auto config =
            port::dpdk_config{.driver = port_info::driver_name(m_idx),
                              .device = port_info::device_name(m_idx),
                              .interface = port_info::interface_name(m_idx)};

        auto link = rte_eth_link{};
        rte_eth_link_get_nowait(m_idx, &link);
        if (link.link_autoneg == ETH_LINK_AUTONEG) {
            config.auto_negotiation = true;
        } else {
            config.auto_negotiation = false;
            config.speed = static_cast<port::link_speed>(link.link_speed);
            config.duplex = (link.link_duplex == ETH_LINK_FULL_DUPLEX
                                 ? port::link_duplex::DUPLEX_FULL
                                 : port::link_duplex::DUPLEX_HALF);
        }

        return (config);
    }

    tl::expected<void, std::string> config(const port::config_data& c)
    {
        return (static_cast<Derived*>(this)->do_config(m_idx, c));
    }

private:
    physical_port(uint16_t idx, std::string_view id)
        : m_idx(idx)
        , m_id(id)
    {}

    friend Derived;

    uint16_t m_idx;
    std::string m_id;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_PHYSICAL_PORT_TCC_ */
