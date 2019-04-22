#include <cassert>

#include "packetio/drivers/dpdk/epoll_poller.h"

namespace icp {
namespace packetio {
namespace dpdk {


union epoll_key {
    struct {
        uint32_t id;
        uint32_t index;
    };
    uintptr_t value;
};

union epoll_key to_epoll_key(pollable_ptr& p)
{
    auto id_visitor = [](auto p) -> uint32_t {
                          return (p->poll_id());
                      };

    return (epoll_key {
            .id = std::visit(id_visitor, p),
            .index = static_cast<uint32_t>(p.index()),
        });
}

bool epoll_poller::add(pollable_ptr q)
{
    auto key = to_epoll_key(q);
    auto add_visitor = [&](auto q) -> bool {
                           return (q->add(RTE_EPOLL_PER_THREAD,
                                          reinterpret_cast<void*>(key.value)));
                       };

    if (!std::visit(add_visitor, q)) return (false);

    m_queues.emplace(key.value, q);
    m_events.reserve(m_queues.size());

    return (true);
}

bool epoll_poller::del(pollable_ptr q)
{
    auto key = to_epoll_key(q);
    auto del_visitor = [&](auto q) -> bool {
                           return (q->del(RTE_EPOLL_PER_THREAD,
                                          reinterpret_cast<void*>(key.value)));
                       };

    if (!std::visit(del_visitor, q)) return (false);

    m_queues.erase(key.value);
    m_events.reserve(m_queues.size());

    return (true);
}

const std::vector<pollable_ptr>& epoll_poller::poll(int timeout_ms)
{
    m_events.clear();

    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    for (int i = 0; i < n; i++) {
        auto result = m_queues.find(reinterpret_cast<uintptr_t>(events[i].epdata.data));
        assert(result != m_queues.end());  /* should never happen */
        auto queue = result->second;
        m_events.emplace_back(queue);
    }

    return (m_events);
}

bool epoll_poller::wait_for_interrupt(int timeout_ms)
{
    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    return (n > 0);
}

}
}
}
