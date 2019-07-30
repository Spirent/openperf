#include <cassert>

#include "packetio/workers/dpdk/callback.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "packetio/workers/dpdk/tx_queue.h"
#include "packetio/workers/dpdk/tx_scheduler.h"
#include "packetio/workers/dpdk/zmq_socket.h"
#include "packetio/workers/dpdk/epoll_poller.h"

namespace icp::packetio::dpdk::worker {

/**
 * Templated function to retrieve the index for a specific type in a variant.
 */
template <typename VariantType, typename T, std::size_t index = 0>
constexpr size_t variant_index() {
    if constexpr (std::is_same_v<std::variant_alternative_t<index, VariantType>, T>) {
        return (index);
    } else {
        return (variant_index<VariantType, T, index + 1>());
    }
}

/*
 * Here be some pointer hacking!
 * We can stuff at most 64 bits of data into an epoll event, so we tag the
 * unused upper bits of our pointer with the variant index.  This allows
 * us to return the proper variant object when we are polled without having
 * to do any lookups.
 */
static constexpr int ptr_shift = 64 - 8;
static constexpr uintptr_t ptr_mask = 0x00ffffffffffffff;

void* to_epoll_ptr(task_ptr ptr)
{
    static_assert(std::variant_size_v<task_ptr> < 255);
    auto tag_ptr_visitor = [](auto task) {
                               auto idx = variant_index<task_ptr, decltype(task)>();
                               return (static_cast<uintptr_t>(idx) << ptr_shift
                                       | (reinterpret_cast<uintptr_t>(task) & ptr_mask));
                           };

    return (reinterpret_cast<void*>(std::visit(tag_ptr_visitor, ptr)));
}

task_ptr to_task_ptr(void* ptr)
{
    auto idx = reinterpret_cast<uintptr_t>(ptr) >> ptr_shift;
    switch (idx) {
    case variant_index<task_ptr, callback*>():
        return (reinterpret_cast<callback*>(reinterpret_cast<uintptr_t>(ptr) & ptr_mask));
    case variant_index<task_ptr, rx_queue*>():
        return (reinterpret_cast<rx_queue*>(reinterpret_cast<uintptr_t>(ptr) & ptr_mask));
    case variant_index<task_ptr, tx_queue*>():
        return (reinterpret_cast<tx_queue*>(reinterpret_cast<uintptr_t>(ptr) & ptr_mask));
    case variant_index<task_ptr, tx_scheduler*>():
        return (reinterpret_cast<tx_scheduler*>(reinterpret_cast<uintptr_t>(ptr) & ptr_mask));
    case variant_index<task_ptr, zmq_socket*>():
        return (reinterpret_cast<zmq_socket*>(reinterpret_cast<uintptr_t>(ptr) & ptr_mask));
    default:
        throw std::runtime_error("Unhandled task_ptr variant; fix me!");
    }
}

epoll_poller::epoll_poller()
{
    m_events.reserve(max_events);
}

bool epoll_poller::add(task_ptr var)
{
    auto add_visitor = [&](auto task) -> bool {
                           return (task->add(RTE_EPOLL_PER_THREAD, to_epoll_ptr(var)));
                       };

    return (std::visit(add_visitor, var));
}

bool epoll_poller::del(task_ptr var)
{
    auto del_visitor = [&](auto task) -> bool {
                           return (task->del(RTE_EPOLL_PER_THREAD, to_epoll_ptr(var)));
                       };

    return (std::visit(del_visitor, var));
}

const std::vector<task_ptr>& epoll_poller::poll(int timeout_ms)
{
    m_events.clear();

    std::array<struct rte_epoll_event, max_events> events;
    auto n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), events.size(), timeout_ms);
    for (int i = 0; i < n; i++) {
        m_events.push_back(to_task_ptr(events[i].epdata.data));
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
