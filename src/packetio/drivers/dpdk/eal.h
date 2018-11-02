#ifndef _ICP_PACKETIO_DPDK_EAL_H_
#define _ICP_PACKETIO_DPDK_EAL_H_

#include <any>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "packetio/generic_port.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/worker_api.h"
#include "packetio/memory/dpdk/pool_allocator.h"

namespace icp {
namespace packetio {

namespace port {
class generic_port;
}

namespace dpdk {

class eal
{
public:
    eal(void *context, std::vector<std::string> args);
    ~eal();

    /* environment is movable */
    eal& operator=(eal&& other)
    {
        if (this != &other) {
            m_initialized = other.m_initialized;
            other.m_initialized = false;
            m_allocator = std::move(other.m_allocator);
            m_workers = std::move(other.m_workers);
            m_bond_ports = std::move(other.m_bond_ports);
            m_switch = std::move(other.m_switch);
        }
        return (*this);
    }

    eal (eal&& other)
        : m_initialized(other.m_initialized)
        , m_allocator(std::move(other.m_allocator))
        , m_workers(std::move(other.m_workers))
        , m_bond_ports(std::move(other.m_bond_ports))
        , m_switch(std::move(other.m_switch))
    {
        other.m_initialized = false;
    }

    /* environment is non-copyable */
    eal(const eal&) = delete;
    eal& operator=(const eal&&) = delete;

    void start();
    void stop();

    static std::vector<int> port_ids();
    std::optional<port::generic_port> port(int id) const;
    tl::expected<int, std::string> create_port(const port::config_data& config);
    tl::expected<void, std::string> delete_port(int id);

    void add_interface(int id, std::any interface);
    void del_interface(int id, std::any interface);

private:
    bool m_initialized;
    std::unique_ptr<pool_allocator> m_allocator;
    std::unique_ptr<worker::client> m_workers;
    std::unordered_map<int, std::string> m_bond_ports;
    std::shared_ptr<vif_switch<netif*>> m_switch;
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_EAL_H_ */
