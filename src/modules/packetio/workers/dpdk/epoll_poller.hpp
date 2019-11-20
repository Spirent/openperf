#ifndef _OP_PACKETIO_DPDK_EPOLL_POLLER_HPP_
#define _OP_PACKETIO_DPDK_EPOLL_POLLER_HPP_

#include <unordered_map>
#include <variant>
#include <vector>

#include "packetio/workers/dpdk/worker_api.hpp"

namespace openperf::packetio::dpdk::worker {

class epoll_poller {
    std::vector<task_ptr> m_events;
    static constexpr size_t max_events = 128;

public:
    epoll_poller();
    ~epoll_poller() = default;

    bool add(task_ptr p);
    bool del(task_ptr p);

    const std::vector<task_ptr>& poll(int timeout_ms = -1);

    bool wait_for_interrupt(int timeout_ms = -1) const;
};

}

#endif /* _OP_PACKETIO_DPDK_EPOLL_POLLER_HPP_ */
