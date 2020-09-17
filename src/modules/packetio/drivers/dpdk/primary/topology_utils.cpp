#include <numeric>

#include "packetio/drivers/dpdk/topology_utils.hpp"

namespace openperf::packetio::dpdk::topology {

std::vector<uint16_t> get_ports()
{
    auto ids = std::vector<uint16_t>{};
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        ids.emplace_back(port_id);
    }

    return (ids);
}

std::vector<uint16_t> get_tx_queues(uint16_t port_id)
{
    auto queues = std::vector<uint16_t>(port_info::tx_queue_count(port_id));
    std::iota(std::begin(queues), std::end(queues), 0);
    return (queues);
}

std::vector<queue::descriptor>
queue_distribute(const std::vector<uint16_t>& port_ids)
{
    return (index_queue_distribute(port_ids));
}

} // namespace openperf::packetio::dpdk::topology
