#ifndef _ICP_PACKETIO_DPDK_TOPOLOGY_UTILS_H_
#define _ICP_PACKETIO_DPDK_TOPOLOGY_UTILS_H_

#include <unordered_map>

#include "packetio/drivers/dpdk/queue_utils.h"
#include "packetio/drivers/dpdk/model/port_info.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace topology {

/**
 * Use the currently available ports, cores, and NUMA architecture to
 * determine a reasonable logical core to run the stack thread.
 */
unsigned get_stack_lcore_id();

/**
 * Generate a vector of queue descriptors based on the specified
 * port configuration.  This distribution takes into account the
 * number of available cores, their numa node/socket id, and
 * port locality.
 */
std::vector<queue::descriptor>
queue_distribute(const std::vector<model::port_info>& port_info, bool single_core);

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

    auto max = std::max_element(begin(node_counts), end(node_counts),
                                [](count_map::value_type& a, count_map::value_type& b) {
                                    return (a.second < b.second);
                                });

    return (max->first);
}

}
}
}
}

#endif /* _ICP_PACKETIO_DPDK_TOPOLOGY_UTILS_H_ */
