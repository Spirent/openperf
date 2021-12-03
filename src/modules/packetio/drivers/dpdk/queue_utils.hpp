#ifndef _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_
#define _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_

#include <cstdint>
#include <map>
#include <vector>

namespace openperf {

namespace core {
class cpuset;
}

namespace packetio::dpdk::queue {

enum class queue_direction { NONE = 0, RX, TX };

/**
 * Structure describing how to configure port queues
 */
struct descriptor
{
    uint16_t worker_id; /**< worker id that should handle the specified queue */
    uint16_t port_id;   /**< queue's port id */
    uint16_t queue_id;  /**< queue's queue id */
    queue_direction direction; /**< operational mode of queue for worker */
};

/**
 * Adjust the number of queues to a reasonable value based on the number
 * of ports and threads.
 */
uint16_t suggested_queue_count(uint16_t nb_ports,
                               uint16_t nb_queues,
                               uint16_t nb_threads);

/**
 * Generate a distribution of port queues among the specified number of
 * {rx,tx} workers.
 *
 * @param[in] port_indexes
 *   A vector containing the port indexes that need queue distribution
 * @param[in] mask
 *   bit mask indicating workers available for servicing port queues
 * @return
 *   A vector containing queue assignments for workers
 */
std::vector<descriptor>
distribute_queues(const std::vector<uint16_t>& port_indexes,
                  const core::cpuset& mask);

struct count
{
    uint16_t rx; /**< number of rx queues */
    uint16_t tx; /**< number of tx queues */
};

/**
 * Generate a map of port index --> queue counts based on assignments
 * in the descriptor vector.
 *
 * @param[in] descriptors
 *   A vector containing queue descriptors
 * @return
 *   A map of port indexes to queue counts
 */
std::map<uint16_t, count>
get_port_queue_counts(const std::vector<descriptor>& descriptors);

} // namespace packetio::dpdk::queue
} // namespace openperf

#endif /* _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_ */
