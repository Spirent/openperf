#ifndef _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
#define _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_

#include <atomic>
#include <unordered_map>
#include <condition_variable>

namespace openperf::framework::generator::internal {

class feedback_tracker
{
private:
    std::mutex m_mutex;
    std::atomic_int m_counter = 0;
    std::condition_variable m_condition;

public:
    feedback_tracker() = default;
    feedback_tracker(const feedback_tracker&) = delete;

    void wait();
    void init(int count);
    void count_down();
};

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
