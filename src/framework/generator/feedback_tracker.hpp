#ifndef _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
#define _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace openperf::framework::generator::internal {

/**
 * This class implements a count down latch mechanism to allow one thread
 * to wait() while other threads count_down() the counter to 0 which
 * unblocks thre waiting thread.
 *
 * FIXME: This class uses a condition_variable and mutex lock.
 *        OpenPerf code should avoid using locks, so code using
 *        this class should be modifed to use lockfree or
 *        ZMQ message based synchronization/wait.
 */
class feedback_tracker
{
private:
    std::mutex m_mutex;
    /* XXX: include/cross compile issue here with std::atomic_int. */
    std::atomic<int> m_counter = 0;
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
