#include "feedback_tracker.hpp"

namespace openperf::framework::generator::internal {

void feedback_tracker::countdown(internal::operation_t op)
{
    if (m_operation == op) {
        m_counter--;
        m_condition.notify_all();
    }
}

void feedback_tracker::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return m_counter <= 0; });
    m_operation = operation_t::NOOP;
}

void feedback_tracker::init(internal::operation_t op, int count)
{
    m_operation = op;
    m_counter = count;
}

} // namespace openperf::framework::generator::internal
