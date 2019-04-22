#ifndef _ICP_PACKETIO_DPDK_EPOLL_POLLER_H_
#define _ICP_PACKETIO_DPDK_EPOLL_POLLER_H_

#include <unordered_map>
#include <variant>
#include <vector>

#include "packetio/drivers/dpdk/rx_queue.h"
#include "packetio/drivers/dpdk/tx_queue.h"
#include "packetio/drivers/dpdk/zmq_socket.h"

namespace icp {
namespace packetio {
namespace dpdk {

typedef std::variant<rx_queue*, tx_queue*, zmq_socket*> pollable_ptr;

class epoll_poller {
    std::unordered_map<uintptr_t, pollable_ptr> m_queues;
    std::vector<pollable_ptr> m_events;
    static constexpr size_t max_events = 128;

public:
    epoll_poller() = default;
    ~epoll_poller() = default;

    bool add(pollable_ptr p);
    bool del(pollable_ptr p);

    const std::vector<pollable_ptr>& poll(int timeout_ms = -1);

    bool wait_for_interrupt(int timeout_ms = -1);
};

}
}
}
#endif /* _ICP_PACKETIO_DPDK_EPOLL_POLLER_H_ */
