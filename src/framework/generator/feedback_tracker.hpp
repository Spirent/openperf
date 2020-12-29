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
    struct map_item_t
    {
        std::condition_variable condition;
        std::atomic_int counter = 0;
    };

    using item_ptr = std::unique_ptr<map_item_t>;

private:
    size_t m_limit = 0;
    std::unordered_map<operation_t, item_ptr> m_vars;

public:
    feedback_tracker();
    feedback_tracker(const feedback_tracker&) = delete;

    void limit(size_t value);
    void countdown(internal::operation_t op);

    size_t limit() const { return m_limit; }
    void wait(internal::operation_t op) const;
    void wait(internal::operation_t op, int count) const;
};

} // namespace openperf::framework::generator::internal

#endif // _OP_FRAMEWORK_GENERATOR_FEEDBACK_TRACKER_HPP_
