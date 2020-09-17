#include <algorithm>
#include <cassert>
#include <numeric>
#include <set>
#include <vector>

#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "packetio/drivers/dpdk/topology_utils.hpp"
#include "core/op_log.h"

namespace openperf::packetio::dpdk::topology {

using cores_by_id = std::map<unsigned, core_mask>;
using ports_by_id = std::map<unsigned, std::vector<unsigned>>;

static core_mask dpdk_mask()
{
    auto mask = core_mask{};
    auto lcore_id = 0U;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        mask.set(lcore_id);
    }
    return (mask);
}

/*
 * Distribute port queues to workers while taking into account NUMA
 * relationships.  This is a simple and probably non-optimal solution to the
 * problem.  The best solution for users is to run one OpenPerf node per NUMA
 * node instance.  But if a user chooses not to do that, this will at least
 * allow everything to work.
 */
std::vector<queue::descriptor>
index_queue_distribute(const std::vector<uint16_t>& port_ids)
{
    /* If we have multiple cores, assign the stack to a dedicated one. */
    unsigned stack_lcore =
        (rte_lcore_count() <= 2 ? std::numeric_limits<unsigned>::max()
                                : get_stack_lcore_id());

    /*
     * Generate a node -> core map so that we can easily tell which cores are
     * on which node.
     */
    const auto mask = config::rx_core_mask().value_or(dpdk_mask())
                      | config::tx_core_mask().value_or(dpdk_mask());
    cores_by_id nodes;
    unsigned lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        if (lcore_id == stack_lcore || !mask[lcore_id]) continue;
        nodes[rte_lcore_to_socket_id(lcore_id)].set(lcore_id);
    }

    /*
     * Generate two NUMA maps for ports.  Matched ports contain ports
     * with cores in the same NUMA node.  Unmatched ports contain ports/nodes
     * where we have no associated workers.
     */
    ports_by_id matched_port_nodes, unmatched_port_nodes;
    std::for_each(
        std::begin(port_ids), std::end(port_ids), [&](const auto& port_id) {
            const auto socket_id = port_info::socket_id(port_id);
            if (nodes.count(socket_id)) {
                matched_port_nodes[socket_id].push_back(port_id);
            } else {
                unmatched_port_nodes[socket_id].push_back(port_id);
            }
        });

    /* Log a warning if we have to assign ports to cores on different nodes
     */
    if (!unmatched_port_nodes.empty()) {
        for (auto& item : unmatched_port_nodes) {
            OP_LOG(OP_LOG_WARNING,
                   "No local cores are available for ports on NUMA socket %u "
                   "(port%s %s)\n",
                   item.first,
                   item.second.size() > 1 ? "s" : "",
                   std::accumulate(
                       begin(item.second),
                       end(item.second),
                       std::string(),
                       [&](const std::string& a, unsigned b) -> std::string {
                           return (a
                                   + (a.length() == 0
                                          ? ""
                                          : item.second.size() == 2
                                                ? " and "
                                                : item.second.back() == b
                                                      ? ", and "
                                                      : ", ")
                                   + std::to_string(b));
                       })
                       .c_str());
        }
    }

    /*
     * Now generate mappings for ports based on NUMA nodes.
     * But first, we need a sorted vector of the NUMA nodes for our workers.
     * We'll use this to help assign port queues without local cores to
     * workers.
     */
    std::vector<unsigned> node_ids;
    for (auto& node : nodes) { node_ids.push_back(node.first); }

    std::vector<queue::descriptor> descriptors;
    for (auto& node : nodes) {
        /*
         * Generate a vector containing the info for all ports associated
         * with this node.
         */
        std::vector<uint16_t> numa_port_ids;
        std::copy_if(begin(port_ids),
                     end(port_ids),
                     std::back_inserter(numa_port_ids),
                     [&](uint16_t port_id) -> bool {
                         return (port_info::socket_id(port_id) == node.first);
                     });

        /*
         * Less obviously, we also want to map some of the ports that don't
         * have local cores to this group of cores.  So, grab all ports that
         * are on NUMA nodes congruent to this node mod the worker node
         * count.
         */
        for (auto& port_node : unmatched_port_nodes) {
            auto node_id = node_ids[port_node.first % nodes.size()];
            if (node_id == node.first) {
                std::copy(begin(port_node.second),
                          end(port_node.second),
                          std::back_inserter(numa_port_ids));
            }
        }

        /* Now, generate a queue distribution for this set of ports and
         * cores */
        auto node_descriptors =
            queue::distribute_queues(numa_port_ids, node.second);

        descriptors.insert(std::end(descriptors),
                           std::begin(node_descriptors),
                           std::end(node_descriptors));
    }

    return (descriptors);
}

static core_mask get_misc_mask()
{
    /* If the user provided explicit cores, then use them */
    if (auto user_misc_mask = config::misc_core_mask()) {
        return (user_misc_mask.value());
    }

    /*
     * Otherwise, remove any user specified rx/tx cores from the set of all
     * cores
     */
    auto misc_mask = dpdk_mask();

    if (auto rx_mask = config::rx_core_mask()) {
        misc_mask &= ~rx_mask.value();
    }

    if (auto tx_mask = config::tx_core_mask()) {
        misc_mask &= ~tx_mask.value();
    }

    return (misc_mask);
}

unsigned get_stack_lcore_id()
{
    /* Generate the set of numa nodes/socket ids from our set of ports */
    std::set<unsigned> node_ids;
    uint16_t port_id = 0;
    RTE_ETH_FOREACH_DEV (port_id) {
        node_ids.insert(rte_eth_dev_socket_id(port_id));
    }

    /* Determine how many cores we can use on each node */
    const auto misc_mask = get_misc_mask();
    cores_by_id nodes;
    int lcore_id = 0;
    RTE_LCORE_FOREACH_SLAVE (lcore_id) {
        if (!misc_mask[lcore_id]) { continue; }
        nodes[rte_lcore_to_socket_id(lcore_id)].set(lcore_id);
    }

    if (nodes.empty()) {
        throw std::runtime_error("No cores available for stack thread!");
    }

    /* Find the numa node with the most available cores */
    auto max = std::max_element(
        begin(nodes),
        end(nodes),
        [](const cores_by_id::value_type& a, const cores_by_id::value_type& b) {
            return (a.second.count() < b.second.count());
        });

    /* Find and return the first allowed core on the largest NUMA node */
    const auto& mask = max->second;
    auto idx = 0U;
    while (idx < mask.size() && !mask[idx]) { ++idx; }
    return (idx);
}

} // namespace openperf::packetio::dpdk::topology
