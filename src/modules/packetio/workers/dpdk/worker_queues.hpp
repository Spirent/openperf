#ifndef _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_
#define _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_

/**
 * @file
 *
 * This singleton structure is the owner of all tx/rx port queues.
 *
 * In order to provide generic transmit functions to our clients, we need to
 * have access to our transmit queues from arbitrary contexts.  Instead of
 * encapsulating that data in std::function, we instead store the data here
 * so that we can provide simple function pointers to clients.  This avoids the
 * overhead associated with type erasure on our transmit path.
 *
 * Additionally, the DPDK is really just a big singleton anyway, so take
 * advantage of that fact.
 */

#include <cstdint>
#include <any>

#include "packetio/drivers/dpdk/queue_utils.hpp"
#include "packetio/workers/dpdk/rxtx_queue_container.hpp"
#include "packetio/workers/dpdk/rx_queue.hpp"
#include "packetio/workers/dpdk/tx_queue.hpp"
#include "utils/singleton.hpp"

struct rte_mbuf;

namespace openperf::packetio::dpdk::worker {

class port_queues : public utils::singleton<port_queues>
{
public:
    void setup(std::any fib, const std::vector<queue::descriptor>& descriptors);
    void unset();

    using queue_container = rxtx_queue_container<rx_queue, tx_queue>;
    const queue_container& operator[](int x) const;

    template <typename T> auto fib() { return std::any_cast<T>(m_fib); }

private:
    std::vector<queue_container> m_queues;
    std::any m_fib;
};

} // namespace openperf::packetio::dpdk::worker

#endif /* _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_ */
