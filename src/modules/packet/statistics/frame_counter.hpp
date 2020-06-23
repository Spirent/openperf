#ifndef _OP_PACKET_STATISTICS_FRAME_COUNTER_HPP_
#define _OP_PACKET_STATISTICS_FRAME_COUNTER_HPP_

#include <chrono>
#include <optional>

#include "timesync/chrono.hpp"

namespace openperf::packet::statistics {

using stat_t = uint64_t;
using clock_t = openperf::timesync::chrono::realtime;

/**
 * Simple counter implementation that tracks the first/last
 * time the counter is updated.
 */
template <typename Clock> struct timestamp_clock
{
    using timestamp = std::chrono::time_point<Clock>;
};

struct frame_counter : timestamp_clock<clock_t>
{
    stat_t count = 0;
    timestamp first_ = timestamp::max();
    timestamp last_ = timestamp::min();

    std::optional<timestamp> first() const
    {
        return (count ? std::optional<timestamp>{first_} : std::nullopt);
    }

    std::optional<timestamp> last() const
    {
        return (count ? std::optional<timestamp>{last_} : std::nullopt);
    }

    frame_counter& operator+=(const frame_counter& rhs)
    {
        count += rhs.count;
        first_ = std::min(first_, rhs.first_);
        last_ = std::max(first_, rhs.last_);

        return (*this);
    }

    friend frame_counter operator+(frame_counter lhs, const frame_counter& rhs)
    {
        lhs += rhs;
        return (lhs);
    }
};

template <typename FrameCounter>
inline void update(FrameCounter& frames,
                   stat_t count,
                   typename FrameCounter::timestamp rx) noexcept
{
    if (!frames.first()) { frames.first_ = rx; }
    frames.count += count;
    frames.last_ = rx;
}

} // namespace openperf::packet::statistics

#endif /* _OP_PACKET_STATISTICS_FRAME_COUNTER_HPP_ */
