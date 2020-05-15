#include <algorithm>
#include <variant>

#include "core/op_log.h"
#include "packetio/internal_client.hpp"
#include "packetio/workers/dpdk/event_loop_adapter.hpp"
#include "packetio/workers/dpdk/epoll_poller.hpp"

namespace openperf::packetio::dpdk::worker {

event_loop_adapter::event_loop_adapter(event_loop_adapter&& other) noexcept
    : m_additions(std::move(other.m_additions))
    , m_deletions(std::move(other.m_deletions))
    , m_runnables(std::move(other.m_runnables))
{}

event_loop_adapter& event_loop_adapter::
operator=(event_loop_adapter&& other) noexcept
{
    if (this != &other) {
        m_additions = std::move(other.m_additions);
        m_deletions = std::move(other.m_deletions);
        m_runnables = std::move(other.m_runnables);
    }
    return (*this);
}

bool event_loop_adapter::update_poller(epoll_poller& poller)
{
    if (m_additions.empty() && m_deletions.empty()) return (true);

    /*
     * Move callbacks on the additions list to the runnable list if we can
     * add them to the poller.
     */
    auto last_addition = std::stable_partition(
        std::begin(m_additions), std::end(m_additions), [&](auto& cbptr) {
            if (poller.add(cbptr.get())) {
                OP_LOG(OP_LOG_DEBUG,
                       "Adding callback %.*s to worker %u\n",
                       static_cast<int>(cbptr->name().length()),
                       cbptr->name().data(),
                       rte_lcore_id());
                return (true);
            }
            return (false);
        });
    std::move(std::begin(m_additions),
              last_addition,
              std::back_inserter(m_runnables));
    m_additions.erase(std::begin(m_additions), last_addition);

    /*
     * Try to remove callbacks from the poller.  If there is an error for
     * some reason, there isn't much we can do about it, so just delete
     * the callback anyway.
     * Note: we have to move the callbacks from m_deletions to a temporary
     * vector before deleting them because the callback's destuctor might
     * trigger more additions to the m_deletions vector. Attempting to modify
     * the vector in the middle of an erase() operation is bad news, yo.
     */
    std::vector<callback_ptr> staging(m_deletions.size());
    for (auto& cbptr : m_deletions) {
        OP_LOG(OP_LOG_DEBUG,
               "Deleting callback %.*s from worker %u\n",
               static_cast<int>(cbptr->name().length()),
               cbptr->name().data(),
               rte_lcore_id());
        poller.del(cbptr.get());
        staging.push_back(std::move(cbptr));
    }
    m_deletions.clear();

    return (m_additions.empty() && m_deletions.empty());
}

bool event_loop_adapter::add_callback(
    std::string_view name,
    event_loop::event_notifier notify,
    event_loop::event_handler&& on_event,
    std::optional<event_loop::delete_handler>&& on_delete,
    std::any&& arg) noexcept
{
    m_additions.push_back(
        std::make_unique<callback>(name,
                                   notify,
                                   std::forward<decltype(on_event)>(on_event),
                                   std::forward<decltype(on_delete)>(on_delete),
                                   std::forward<std::any>(arg)));
    return (true);
}

void event_loop_adapter::del_callback(
    event_loop::event_notifier notify) noexcept
{
    /*
     * Since partition sorts the vector into [true items, false items], we need
     * the inverse function for sorting, as we want the matching item at the
     * end of the vector for easier move/deletion.
     */
    auto not_callback = [&](auto& cbptr) {
        return (cbptr->notifier() != notify);
    };

    /* Move the matching callbacks and move them into our deletions list. */
    auto additions_to_move = std::stable_partition(
        std::begin(m_additions), std::end(m_additions), not_callback);
    std::move(additions_to_move,
              std::end(m_additions),
              std::back_inserter(m_deletions));
    m_additions.erase(additions_to_move, std::end(m_additions));

    auto runnables_to_move = std::stable_partition(
        std::begin(m_runnables), std::end(m_runnables), not_callback);
    std::move(runnables_to_move,
              std::end(m_runnables),
              std::back_inserter(m_deletions));
    m_runnables.erase(runnables_to_move, std::end(m_runnables));
}

} // namespace openperf::packetio::dpdk::worker
