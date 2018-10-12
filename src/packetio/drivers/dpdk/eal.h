#ifndef _ICP_PACKETIO_DPDK_EAL_H_
#define _ICP_PACKETIO_DPDK_EAL_H_

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "packetio/generic_port.h"
#include "memory/dpdk/pool_allocator.h"

namespace icp {
namespace packetio {

namespace port {
class generic_port;
}

namespace dpdk {

class eal
{
public:
    eal(std::vector<std::string> args);
    ~eal();

    /* environment is movable */
    eal& operator=(eal&& other)
    {
        if (this != &other) {
            m_initialized = other.m_initialized;
            other.m_initialized = false;
            m_allocator = std::move(other.m_allocator);
        }
        return (*this);
    }

    eal (eal&& other)
    {
        m_initialized = other.m_initialized;
        other.m_initialized = false;
        m_allocator = std::move(other.m_allocator);
    }

    /* environment is non-copyable */
    eal(const eal&) = delete;
    eal& operator=(const eal&&) = delete;

    static std::vector<int> port_ids();
    std::optional<port::generic_port> port(int id) const;
    tl::expected<int, std::string> create_port(const port::config_data& config);
    void delete_port(int id);

private:
    bool m_initialized;
    std::unique_ptr<pool_allocator> m_allocator;
    std::unordered_map<int, std::string> m_bond_ports;
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_EAL_H_ */
