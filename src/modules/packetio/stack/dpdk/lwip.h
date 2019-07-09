#ifndef _ICP_PACKETIO_STACK_LWIP_H_
#define _ICP_PACKETIO_STACK_LWIP_H_

#include <optional>
#include <string>
#include <map>
#include <vector>

#include "tl/expected.hpp"

#include "core/icp_uuid.h"
#include "packetio/generic_interface.h"
#include "packetio/generic_stack.h"
#include "packetio/stack/dpdk/net_interface.h"

namespace icp::packetio::dpdk {

class lwip
{
public:
    lwip(driver::generic_driver&, workers::generic_workers& workers);
    ~lwip();

    /* stack is movable */
    lwip& operator=(lwip&& other);
    lwip (lwip&& other);

    /* stack is non-copyable */
    lwip(const lwip&) = delete;
    lwip& operator=(const lwip&&) = delete;

    std::string id() const { return "stack0"; }  /* only 1 stack for now */
    std::vector<std::string> interface_ids() const;
    std::optional<interface::generic_interface> interface(std::string_view id) const;

    tl::expected<std::string, std::string> create_interface(const interface::config_data& config);
    void delete_interface(std::string_view id);

    std::unordered_map<std::string, stack::stats_data> stats() const;

private:
    bool m_initialized;
    driver::generic_driver& m_driver;
    workers::generic_workers& m_workers;
    std::vector<std::string> m_tasks;
    std::map<std::string, std::unique_ptr<net_interface>, std::less<>> m_interfaces;
    int m_timerfd;
};

}

#endif /* _ICP_PACKETIO_STACK_LWIP_H_ */
