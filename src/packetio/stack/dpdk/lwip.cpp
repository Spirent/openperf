#include "lwipopts.h"
#include "lwip/tcpip.h"

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/lwip.h"

namespace icp {
namespace packetio {
namespace dpdk {

static void tcpip_init_done(void *arg __attribute__((unused)))
{
    ICP_LOG(ICP_LOG_DEBUG, "TCP/IP thread running on logical core %u\n",
            rte_lcore_id());
}

lwip::lwip(driver::generic_driver& driver)
    : m_driver(driver)
    , m_idx(0)
{
    tcpip_init(tcpip_init_done, nullptr);
}

std::vector<int> lwip::interface_ids() const
{
    std::vector<int> ids;
    for (auto& item : m_interfaces) {
        ids.push_back(item.first);
    }

    return (ids);
}

std::optional<interface::generic_interface> lwip::interface(int id) const
{
    return (m_interfaces.find(id) != m_interfaces.end()
            ? std::make_optional(interface::generic_interface(*m_interfaces.at(id).get()))
            : std::nullopt);
}

tl::expected<int, std::string> lwip::create_interface(const interface::config_data& config)
{
    try {
        auto item = m_interfaces.emplace(
            std::make_pair(m_idx, std::make_unique<net_interface>(m_idx, config)));
        m_idx++;
        return (item.first->first);
    } catch (const std::runtime_error &e) {
        ICP_LOG(ICP_LOG_ERROR, "Interface creation failed: %s\n", e.what());
        return (tl::make_unexpected(e.what()));
    }
}

void lwip::delete_interface(int id)
{
    m_interfaces.erase(id);
}

}
}
}
