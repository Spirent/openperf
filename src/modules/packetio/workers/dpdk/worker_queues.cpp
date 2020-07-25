#include <map>

#include "packetio/workers/dpdk/worker_queues.hpp"

namespace openperf::packetio::dpdk::worker {

void port_queues::setup(std::any fib,
                        const std::vector<queue::descriptor>& descriptors)
{
    m_fib = std::move(fib);

    assert(m_queues.empty());
    m_queues.reserve(RTE_MAX_ETHPORTS);

    for (auto& [idx, queue_count] : queue::get_port_queue_counts(descriptors)) {
        m_queues.emplace(
            begin(m_queues) + idx, idx, queue_count.rx, queue_count.tx);
    }
}

void port_queues::unset() { m_queues.clear(); }

const port_queues::queue_container& port_queues::operator[](int idx) const
{
    return (m_queues[idx]);
}

} // namespace openperf::packetio::dpdk::worker
