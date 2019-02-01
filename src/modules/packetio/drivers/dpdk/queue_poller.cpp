#include <cassert>

#include "packetio/drivers/dpdk/queue_poller.h"

namespace icp {
namespace packetio {
namespace dpdk {

enum class queue_type:uint16_t { NO_QUEUE = 0,
                                 RX_QUEUE = 1,
                                 TX_QUEUE = 2 };
union epoll_key {
    struct {
        uint16_t port_id;
        uint16_t queue_id;
        queue_type type;
    };
    uintptr_t value;
};

union epoll_key to_epoll_key(queue_ptr& q)
{
    auto port_visitor = [](auto q) -> uint16_t {
                            return (q->port_id());
                        };

    auto queue_visitor = [](auto q) -> uint16_t {
                             return (q->queue_id());
                         };

    return (epoll_key {
            .port_id = std::visit(port_visitor, q),
            .queue_id = std::visit(queue_visitor, q),
            .type = (std::holds_alternative<rx_queue*>(q)
                     ? queue_type::RX_QUEUE
                     : queue_type::TX_QUEUE) });
}

bool queue_poller::add(queue_ptr q)
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

bool queue_poller::del(queue_ptr q)
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

const std::vector<queue_ptr>& queue_poller::poll(int timeout_ms)
{
    m_events.clear();

    auto disable_visitor = [](auto q) { q->disable(); };

    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    for (int i = 0; i < n; i++) {
        auto result = m_queues.find(reinterpret_cast<uintptr_t>(events[i].epdata.data));
        assert(result != m_queues.end());  /* should never happen */
        auto queue = result->second;
        std::visit(disable_visitor, queue);
        m_events.emplace_back(queue);
    }

    return (m_events);
}

bool queue_poller::wait_for_interrupt(int timeout_ms)
{
    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    return (n > 0);
}

}
}
}
