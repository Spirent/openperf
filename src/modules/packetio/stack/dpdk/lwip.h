#ifndef _ICP_PACKETIO_STACK_LWIP_H_
#define _ICP_PACKETIO_STACK_LWIP_H_

#include <optional>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

#include "lwipopts.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "tl/expected.hpp"

#include "packetio/generic_driver.h"
#include "packetio/generic_interface.h"
#include "packetio/generic_stack.h"
#include "packetio/stack/dpdk/net_interface.h"

namespace icp {
namespace packetio {
namespace dpdk {

class lwip
{
public:
    lwip(driver::generic_driver& driver);
    ~lwip();

    /* stack is movable */
    lwip& operator=(lwip&& other)
    {
        if (this != &other) {
            m_initialized = other.m_initialized;
            m_interfaces = std::move(other.m_interfaces);
            m_driver = std::move(other.m_driver);
        }
        return (*this);
    }

    lwip (lwip&& other)
        : m_initialized(other.m_initialized)
        , m_driver(other.m_driver)
    {
        other.m_initialized = false;
        m_interfaces = std::move(other.m_interfaces);
    }

    /* stack is non-copyable */
    lwip(const lwip&) = delete;
    lwip& operator=(const lwip&&) = delete;

    int id() const { return 0; }  /* only 1 stack for now */
    std::vector<std::string> interface_ids() const;
    std::optional<interface::generic_interface> interface(std::string_view id) const;
    tl::expected<std::string, std::string> create_interface(const interface::config_data& config);
    void delete_interface(std::string_view id);

    std::unordered_map<std::string, stack::stats_data> stats() const;

private:
    bool m_initialized;
    driver::generic_driver& m_driver;
    std::map<std::string, std::unique_ptr<net_interface>, std::less<>> m_interfaces;
};

}
}
}
#endif /* _ICP_PACKETIO_STACK_LWIP_H_ */
