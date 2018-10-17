#ifndef _ICP_PACKETIO_STACK_LWIP_H_
#define _ICP_PACKETIO_STACK_LWIP_H_

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "lwipopts.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"
#include "tl/expected.hpp"

#include "packetio/generic_driver.h"
#include "packetio/generic_interface.h"
#include "packetio/stack/dpdk/net_interface.h"

namespace icp {
namespace packetio {
namespace dpdk {

class lwip
{
public:
    lwip(driver::generic_driver& driver);
    ~lwip() = default;

    /* stack is movable */
    lwip& operator=(lwip&& other)
    {
        if (this != &other) {
            m_interfaces = std::move(other.m_interfaces);
            m_driver = std::move(other.m_driver);
            m_idx = std::move(other.m_idx);
        }
        return (*this);
    }

    lwip (lwip&& other)
        : m_driver(other.m_driver)
        , m_idx(other.m_idx)
    {
        m_interfaces = std::move(other.m_interfaces);
    }

    /* stack is non-copyable */
    lwip(const lwip&) = delete;
    lwip& operator=(const lwip&&) = delete;

    std::vector<int> interface_ids() const;
    std::optional<interface::generic_interface> interface(int id) const;
    tl::expected<int, std::string> create_interface(int port_id,
                                                    const interface::config_data& config);
    void delete_interface(int id);

private:
    driver::generic_driver& m_driver;
    std::unordered_map<size_t, std::unique_ptr<net_interface>> m_interfaces;
    int m_idx;
};

}
}
}
#endif /* _ICP_PACKETIO_STACK_LWIP_H_ */
