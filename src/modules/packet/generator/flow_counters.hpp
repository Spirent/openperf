#ifndef _OP_PACKET_GENERATOR_FLOW_COUNTERS_HPP_
#define _OP_PACKET_GENERATOR_FLOW_COUNTERS_HPP_

#include <optional>

#include "timesync/chrono.hpp"

namespace openperf::packet::generator::statistics {

using stat_t = uint64_t;
using clock_t = openperf::timesync::chrono::realtime;

template <typename Clock> struct timestamp_clock
{
    using timestamp = std::chrono::time_point<Clock>;
};

struct counter : timestamp_clock<clock_t>
{
    stat_t octet;
    stat_t packet;
    stat_t error;

    timestamp first_ = timestamp::max();
    timestamp last_ = timestamp::min();

    std::optional<timestamp> first() const noexcept
    {
        return (packet ? std::optional<timestamp>{first_} : std::nullopt);
    }

    std::optional<timestamp> last() const noexcept
    {
        return (packet ? std::optional<timestamp>{last_} : std::nullopt);
    }

    counter& operator+=(const counter& rhs)
    {
        octet += octet;
        packet += packet;
        error += error;
        first_ = std::min(first_, rhs.first_);
        last_ = std::max(last_, rhs.last_);

        return (*this);
    }
};

counter operator+(counter lhs, const counter& rhs)
{
    lhs += rhs;
    return (lhs);
}

} // namespace openperf::packet::generator::statistics
#endif /* _OP_PACKET_GENERATOR_FLOW_COUNTERS_HPP_ */
