#ifndef _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_
#define _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_

#include <unordered_map>
#include <variant>
#include <vector>

#include "packetio/drivers/dpdk/rx_queue.h"
#include "packetio/drivers/dpdk/tx_queue.h"

namespace icp {
namespace packetio {
namespace dpdk {

typedef std::variant<rx_queue*, tx_queue*> queue_ptr;

class queue_poller {
    std::unordered_map<uintptr_t, queue_ptr> m_queues;
    std::vector<queue_ptr> m_events;
    static constexpr size_t max_events = 128;

public:
    queue_poller() = default;
    ~queue_poller() = default;

    bool add(queue_ptr q);
    bool del(queue_ptr q);

    const std::vector<queue_ptr>& poll(int timeout_ms = -1);

    bool wait_for_interrupt(int timeout_ms = -1);
};

}
}
}
#endif /* _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_ */
