#ifndef _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_
#define _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_

#include <cstdint>
#include <map>
#include <vector>

#include "packetio/drivers/dpdk/model/core_mask.hpp"

namespace openperf::packetio::dpdk {

namespace model {
class port_info;
}

namespace queue {

enum class queue_mode { NONE = 0, RX, TX, RXTX };

/**
 * Structure describing how to configure port queues
 */
struct descriptor
{
    uint16_t worker_id; /**< worker id that should handle the specified queue */
    uint16_t port_id;   /**< queue's port id */
    uint16_t queue_id;  /**< queue's queue id */
    queue_mode mode;    /**< operational mode of queue for worker */
};

/**
 * Generate a distribution of port queues among the specified number of
 * {rx,tx} workers.
 *
 * @param[in] port_info
 *   A vector containing a port_info entry for every port
 * @param[in] mask
 *   bit mask indicating workers available for servicing port queues
 * @return
 *   A vector containing queue assignments for workers
 */
std::vector<descriptor>
distribute_queues(const std::vector<model::port_info>& port_info,
                  const model::core_mask& mask);

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
std::map<int, count>
get_port_queue_counts(const std::vector<descriptor>& descriptors);

} // namespace queue
} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_QUEUE_UTILS_HPP_ */
