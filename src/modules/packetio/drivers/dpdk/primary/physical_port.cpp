#include "packetio/drivers/dpdk/primary/physical_port.hpp"
#include "packetio/drivers/dpdk/primary/utils.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"

namespace openperf::packetio::dpdk::primary {

physical_port::physical_port(uint16_t idx,
                             std::string_view id,
                             rte_mempool* const mempool)
    : dpdk::physical_port<physical_port>(idx, id)
    , m_mempool(mempool)
{}

tl::expected<void, std::string>
physical_port::do_config(uint16_t port_idx, const port::config_data& config)
{
    if (!std::holds_alternative<port::dpdk_config>(config)) {
        return tl::make_unexpected("Config does not contain dpdk config data");
    }

    const auto& dpdk_config = std::get<port::dpdk_config>(config);

    return (utils::reconfigure_port(port_idx, m_mempool, dpdk_config));
}

} // namespace openperf::packetio::dpdk::primary
