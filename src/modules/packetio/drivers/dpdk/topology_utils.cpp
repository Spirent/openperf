#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>
#include <vector>

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/topology_utils.h"
#include "core/icp_log.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace topology {

using cores_by_id = std::unordered_map<unsigned, std::vector<unsigned>>;

/*
 * Distribute port queues to workers while taking into account NUMA
 * relationships.  This is a simple and probably non-optimal solution to the
 * problem.  The best solution for users is to run one Inception node per NUMA
 * node instance.  But if a user chooses not to do that, this will at least
 * allow everything to work.
 */
std::vector<queue::descriptor>
queue_distribute(const std::vector<model::port_info>& port_info)
{
    /*
     * First, pick the core that should run the stack. We don't want to assign
     * any queues to it.
     */
    auto stack_lcore = get_stack_lcore_id();

    /*
     * Generate a node -> core map so that we can easily tell which cores are
     * on which node.
     */
    cores_by_id nodes;
    unsigned lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        /* No other cores should be running anything */
        assert(rte_eal_get_lcore_state(lcore_id) == WAIT);
        if (lcore_id == stack_lcore) continue;

        nodes[rte_lcore_to_socket_id(lcore_id)].push_back(lcore_id);
    }

    /*
     * Generate two NUMA maps for ports.  Matched ports contain ports
     * with cores in the same NUMA node.  Unmatched ports contain ports/nodes
     * where we have no associated workers.
     */
    cores_by_id matched_port_nodes, unmatched_port_nodes;
    for (auto& info : port_info) {
        if (nodes.find(info.socket_id()) != nodes.end()) {
            matched_port_nodes[info.socket_id()].push_back(info.id());
        } else {
            unmatched_port_nodes[info.socket_id()].push_back(info.id());
        }
    }

    /* Log a warning if we have to assign ports to cores on different nodes */
    if (!unmatched_port_nodes.empty()) {
        for (auto& item : unmatched_port_nodes) {
            ICP_LOG(ICP_LOG_WARNING,
                    "No local cores are available for ports on NUMA socket %u (port%s %s)\n",
                    item.first,
                    item.second.size() > 1 ? "s" : "",
                    std::accumulate(begin(item.second), end(item.second), std::string(),
                                    [&](const std::string &a, unsigned b) -> std::string {
                                        return (a
                                                + (a.length() == 0 ? ""
                                                   : item.second.size() == 2 ? " and "
                                                   : item.second.back() == b ? ", and "
                                                   : ", ")
                                                + std::to_string(b));
                                    }).c_str());
        }
    }

    /*
     * Now generate mappings for ports based on NUMA nodes.
     * But first, we need a sorted vector of the NUMA nodes for our workers.
     * We'll use this to help assign port queues without local cores to workers.
     */
    std::vector<unsigned> node_ids;
    for (auto& node : nodes) {
        node_ids.push_back(node.first);
    }
    std::sort(begin(node_ids), end(node_ids));

    std::vector<queue::descriptor> descriptors;
    for (auto& node : nodes) {
        /*
         * Generate a vector containing the info for all ports associated with
         * this node.
         */
        std::vector<model::port_info> numa_port_info;
        std::copy_if(begin(port_info), end(port_info), std::back_inserter(numa_port_info),
                     [&](const model::port_info& info) -> bool {
                         return (info.socket_id() == node.first);
                     });

        /*
         * Less obviously, we also want to map some of the ports that don't have
         * local cores to this group of cores.  So, grab all ports that are on
         * NUMA nodes congruent to this node mod the worker node count.
         */
        for (auto& port_node : unmatched_port_nodes) {
            auto node_id = node_ids[port_node.first % nodes.size()];
            if (node_id == node.first) {
                std::copy(begin(port_node.second), end(port_node.second),
                          std::back_inserter(numa_port_info));
            }
        }

        /* Now, generate a queue distribution for this set of ports and cores */
        auto node_descriptors = queue::distribute_queues(numa_port_info, node.second.size());

        /*
         * Finally, update the descriptor so that the worker id points to the
         * correct core id for that numa node.  Then add it to our final list
         * of queue descriptors.
         */
        std::transform(begin(node_descriptors), end(node_descriptors), std::back_inserter(descriptors),
                       [&](const queue::descriptor& d) -> queue::descriptor {
                           return (queue::descriptor{
                                   .worker_id = static_cast<uint16_t>(
                                       node.second[d.worker_id]),
                                   .port_id = d.port_id,
                                   .queue_id = d.queue_id,
                                   .mode = d.mode });
                       });
    }

    return (descriptors);
}

unsigned get_stack_lcore_id()
{
    /* Generate the set of numa nodes/socket ids from our set of ports */
    std::set<unsigned> node_ids;
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV(port_id) {
        node_ids.insert(rte_eth_dev_socket_id(port_id));
    }

    /* Determine how many cores we can use on each node */
    cores_by_id nodes;
    int lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        nodes[rte_lcore_to_socket_id(lcore_id)].push_back(lcore_id);
    }

    /* Find the numa node with the most cores */
    auto max = std::max_element(begin(nodes), end(nodes),
                                [](const cores_by_id::value_type& a,
                                   const cores_by_id::value_type& b) {
                                    return (a.second.size() < b.second.size());
                                });

    /* Return the first available core */
    assert(!max->second.empty());
    return (max->second.front());
}

}
}
}
}
