#include "feedback_tracker.hpp"

namespace openperf::framework::generator::internal {

feedback_tracker::feedback_tracker()
{
    static const auto list = {
        operation_t::PAUSE,
        operation_t::STOP,
        operation_t::RESUME,
        operation_t::RESET,
        operation_t::READY,
    };

    for (auto operation : list) {
        m_vars.emplace(operation, std::make_unique<map_item_t>());
    }
}

void feedback_tracker::limit(size_t value)
{
    for (auto& [op, item] : m_vars) {
        item->counter += value - m_limit;
        if (item->counter <= 0) {
            item->counter = value;
            item->condition.notify_all();
        }
    }

    m_limit = value;
}

void feedback_tracker::countdown(internal::operation_t op)
{
    if (auto it = m_vars.find(op); it != m_vars.end()) {
        it->second->counter--;
        if (it->second->counter <= 0) {
            it->second->counter = m_limit;
            it->second->condition.notify_all();
        }
    }
}

void feedback_tracker::wait(internal::operation_t op) const
{
    if (auto it = m_vars.find(op); it != m_vars.end()) {
        std::mutex mutex;
        std::unique_lock<std::mutex> lock(mutex);
        it->second->condition.wait(lock);
    }
}

void feedback_tracker::wait(internal::operation_t op, int count) const
{
    if (auto it = m_vars.find(op); it != m_vars.end()) {
        it->second->counter = count;
        wait(op);
    }
}

} // namespace openperf::framework::generator::internal
