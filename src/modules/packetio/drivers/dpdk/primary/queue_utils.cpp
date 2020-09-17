#include "packetio/drivers/dpdk/queue_utils.hpp"

namespace openperf::packetio::dpdk::queue {

/**
 * This looks complicated, but really we just try to assign a reasonable number
 * of queues based on our port/queue/thread constraints.
 */
uint16_t suggested_queue_count(uint16_t nb_ports,
                               uint16_t nb_queues,
                               uint16_t nb_threads)
{
    return (
        nb_threads == 1
            ? 1
            : nb_ports < nb_threads
                  ? (nb_threads % nb_ports == 0
                         ? std::min(
                             nb_queues,
                             static_cast<uint16_t>(nb_threads / nb_ports))
                         : std::min(nb_queues, nb_threads))
                  : std::min(nb_queues, static_cast<uint16_t>(nb_threads - 1)));
}

} // namespace openperf::packetio::dpdk::queue
