#include "packetio/drivers/dpdk/queue_utils.hpp"

namespace openperf::packetio::dpdk::queue {

/* Secondary process just has to accept whatever already exists */
uint16_t suggested_queue_count(uint16_t, uint16_t nb_queues, uint16_t)
{
    return (nb_queues);
}

} // namespace openperf::packetio::dpdk::queue
