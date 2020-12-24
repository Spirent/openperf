#ifndef _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
#define _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_

#include <atomic>
#include <unordered_map>
#include <condition_variable>

namespace openperf::framework::generator::internal {

enum class operation_t {
    NOOP = 0,
    PAUSE,
    RESUME,
    RESET,
    STOP,
    STATISTICS,
    INIT,
    READY,
};

class feedback_tracker
{
private:
    std::atomic_int m_counter = 0;
    internal::operation_t m_operation = operation_t::NOOP;

    std::mutex m_mutex;
    std::condition_variable m_condition;

public:
    feedback_tracker() = default;
    feedback_tracker(const feedback_tracker&) = delete;

    void wait();
    void init(internal::operation_t op, int count);
    void countdown(internal::operation_t op);
};

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
