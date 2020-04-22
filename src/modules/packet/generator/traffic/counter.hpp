#ifndef _OP_PACKET_GENERATOR_TRAFFIC_COUNTER_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_COUNTER_HPP_

#include <optional>

#include "timesync/chrono.hpp"

namespace openperf::packet::generator::traffic {

using stat_t = uint64_t;
using clock_t = openperf::timesync::chrono::realtime;

template <typename Clock> struct timestamp_clock
{
    using timestamp = std::chrono::time_point<Clock>;
};

struct counter : timestamp_clock<clock_t>
{
    stat_t octet = 0;
    stat_t packet = 0;

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
        octet += rhs.octet;
        packet += rhs.packet;
        first_ = std::min(first_, rhs.first_);
        last_ = std::max(last_, rhs.last_);

        return (*this);
    }
};

inline counter operator+(counter lhs, const counter& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Counter>
inline void update(Counter& counter,
                   stat_t nb_octets,
                   typename Counter::timestamp tx) noexcept
{
    if (!counter.first()) { counter.first_ = tx; }
    counter.octet += nb_octets;
    counter.packet++;
    counter.last_ = tx;
}

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_COUNTER_HPP_ */
