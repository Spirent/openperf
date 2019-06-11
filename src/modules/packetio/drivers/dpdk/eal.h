#ifndef _ICP_PACKETIO_DPDK_EAL_H_
#define _ICP_PACKETIO_DPDK_EAL_H_

#include <any>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "packetio/generic_driver.h"
#include "packetio/generic_port.h"
#include "packetio/drivers/dpdk/rxtx_queue_container.h"
#include "packetio/drivers/dpdk/rx_queue.h"
#include "packetio/drivers/dpdk/tx_queue.h"
#include "packetio/drivers/dpdk/worker_api.h"
#include "packetio/memory/dpdk/pool_allocator.h"
#include "packetio/vif_map.h"

namespace icp {
namespace packetio {

namespace port {
class generic_port;
}

namespace dpdk {

class eal
{
public:
    eal(void *context, std::vector<std::string> args, int test_portpairs = 0);
    ~eal();

    /* environment is movable */
    eal& operator=(eal&& other);
    eal(eal&& other);

    /* environment is non-copyable */
    eal(const eal&) = delete;
    eal& operator=(const eal&&) = delete;

    void start();
    void stop();

    static std::vector<int> port_ids();
    std::optional<port::generic_port> port(int id) const;

    driver::tx_burst tx_burst_function(int port) const;

    tl::expected<int, std::string> create_port(const port::config_data& config);
    tl::expected<void, std::string> delete_port(int id);

    tl::expected<void, int> attach_port_sink(std::string_view id, pga::generic_sink& sink);
    void detach_port_sink(std::string_view id, pga::generic_sink& sink);

    tl::expected<void, int> attach_port_source(std::string_view id, pga::generic_source& source);
    void detach_port_source(std::string_view id, pga::generic_source& source);

    void add_interface(int id, std::any interface);
    void del_interface(int id, std::any interface);

private:
    bool m_initialized;
    std::unique_ptr<pool_allocator> m_allocator;
    std::unique_ptr<worker::client> m_workers;
    std::shared_ptr<vif_map<netif>> m_switch;
    std::unordered_map<int, std::string> m_bond_ports;

    typedef rxtx_queue_container<rx_queue, tx_queue> port_queues;
    typedef std::unique_ptr<port_queues> port_queues_ptr;
    std::vector<port_queues_ptr> m_port_queues;

};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_EAL_H_ */
