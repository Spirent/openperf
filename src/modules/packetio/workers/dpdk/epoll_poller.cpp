#include <cassert>

#include "packetio/workers/dpdk/callback.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "packetio/workers/dpdk/tx_queue.h"
#include "packetio/workers/dpdk/zmq_socket.h"
#include "packetio/workers/dpdk/epoll_poller.h"

namespace icp::packetio::dpdk::worker {

union epoll_key {
    struct {
        uint32_t id;
        uint32_t index;
    };
    uintptr_t value;
};

union epoll_key to_epoll_key(task_ptr& p)
{
    auto id_visitor = [](auto p) -> uint32_t {
                          return (p->poll_id());
                      };

    return (epoll_key {
            .id = std::visit(id_visitor, p),
            .index = static_cast<uint32_t>(p.index()),
        });
}

bool epoll_poller::add(task_ptr t)
{
    auto key = to_epoll_key(t);
    auto add_visitor = [&](auto t) -> bool {
                           return (t->add(RTE_EPOLL_PER_THREAD,
                                          reinterpret_cast<void*>(key.value)));
                       };

    if (!std::visit(add_visitor, t)) return (false);

    m_tasks.emplace(key.value, t);
    m_events.reserve(m_tasks.size());

    return (true);
}

bool epoll_poller::del(task_ptr t)
{
    auto key = to_epoll_key(t);
    auto del_visitor = [&](auto t) -> bool {
                           return (t->del(RTE_EPOLL_PER_THREAD,
                                          reinterpret_cast<void*>(key.value)));
                       };

    if (!std::visit(del_visitor, t)) return (false);

    m_tasks.erase(key.value);
    m_events.reserve(m_tasks.size());

    return (true);
}

const std::vector<task_ptr>& epoll_poller::poll(int timeout_ms)
{
    m_events.clear();

    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    for (int i = 0; i < n; i++) {
        auto result = m_tasks.find(reinterpret_cast<uintptr_t>(events[i].epdata.data));
        assert(result != m_tasks.end());  /* should never happen */
        auto task = result->second;
        m_events.emplace_back(task);
    }

    return (m_events);
}

bool epoll_poller::wait_for_interrupt(int timeout_ms) const
{
    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    return (n > 0);
}

}
