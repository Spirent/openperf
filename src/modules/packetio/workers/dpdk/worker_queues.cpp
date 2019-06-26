#include <map>

#include "packetio/workers/dpdk/worker_queues.h"

namespace icp::packetio::dpdk::worker {

struct queue_count {
    uint16_t rx;
    uint16_t tx;
};

std::map<int, queue_count> get_port_queue_counts(const std::vector<queue::descriptor>& descriptors)
{
    std::map<int, queue_count> port_queue_counts;

    for (auto& d : descriptors) {
        if (port_queue_counts.find(d.port_id) == port_queue_counts.end()) {
            port_queue_counts.emplace(d.port_id, queue_count{0, 0});
        }

        auto& queue_count = port_queue_counts[d.port_id];

        switch (d.mode) {
        case queue::queue_mode::RX:
            queue_count.rx++;
            break;
        case queue::queue_mode::TX:
            queue_count.tx++;
            break;
        case queue::queue_mode::RXTX:
            queue_count.rx++;
            queue_count.tx++;
            break;
        default:
            break;
        }
    }

    return (port_queue_counts);
}

void port_queues::setup(const std::vector<queue::descriptor>& descriptors)
{
    assert(m_queues.empty());
    m_queues.reserve(RTE_MAX_ETHPORTS);

    for (auto& [idx, queue_count] : get_port_queue_counts(descriptors)) {
        m_queues.emplace(begin(m_queues) + idx, idx, queue_count.rx, queue_count.tx);
    }
}

void port_queues::unset()
{
    m_queues.clear();
}

const port_queues::queue_container& port_queues::operator[](int idx) const
{
    return (m_queues[idx]);
}

}
