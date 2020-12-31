#include "feedback_tracker.hpp"

namespace openperf::framework::generator::internal {

void feedback_tracker::count_down()
{
    if (m_counter > 0) {
        m_counter--;
        m_condition.notify_all();
    }
}

void feedback_tracker::wait()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_condition.wait(lock, [this] { return m_counter <= 0; });
}

void feedback_tracker::init(int count) { m_counter = count; }

} // namespace openperf::framework::generator::internal
