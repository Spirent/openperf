#ifndef _OP_PACKETIO_DPDK_TOPOLOGY_UTILS_HPP_
#define _OP_PACKETIO_DPDK_TOPOLOGY_UTILS_HPP_

#include <algorithm>
#include <unordered_map>

#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/drivers/dpdk/queue_utils.hpp"

namespace openperf::packetio::dpdk::topology {

/**
 * Use the currently available ports, cores, and NUMA architecture to
 * determine a reasonable logical core to run the stack thread.
 * Note: one might not be present.
 */
std::optional<unsigned> get_stack_lcore_id();

std::vector<uint16_t> get_ports();

/**
 * Use the port queue counts to generate a vector of queue
 * descriptors based on available cores, numa nodes, and port
 * locality. These descriptors are index based and DO NOT
 * necessarily reflect the actual queues in use. Use the function
 * below instead to get actual, usable queue indexes.
 */
std::vector<queue::descriptor>
index_queue_distribute(const std::vector<uint16_t>& port_ids);

/**
 * Generate a vector of queue descriptors based on the specified
 * port configuration.  This distribution takes into account the
 * number of available cores, their numa node/socket id, and
 * port locality.
 */
std::vector<queue::descriptor>
queue_distribute(const std::vector<uint16_t>& port_ids);

/**
 * Retrieve a vector of transmit queue indexes for the given port.
 * When running as a secondary process, the queue id's might not
 * be the range [0, count).
 */
std::vector<uint16_t> get_tx_queues(uint16_t port_id);

/**
 * Determine the most common NUMA node connected to the most ports
 * in the set of port_ids.
 */
template <typename Container>
unsigned get_most_common_numa_node(const Container& port_ids)
{
    using count_map = std::unordered_map<unsigned, unsigned>;
    count_map node_counts;
    for (auto id : port_ids) {
        auto socket = rte_eth_dev_socket_id(id);
        node_counts[socket]++;
    }

    auto max = std::max_element(
        begin(node_counts),
        end(node_counts),
        [](count_map::value_type& a, count_map::value_type& b) {
            return (a.second < b.second);
        });

    return (max->first);
}

} // namespace openperf::packetio::dpdk::topology

#endif /* _OP_PACKETIO_DPDK_TOPOLOGY_UTILS_HPP_ */
