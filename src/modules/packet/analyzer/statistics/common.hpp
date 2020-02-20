#ifndef _OP_ANALYZER_STATISTICS_COMMON_HPP_
#define _OP_ANALYZER_STATISTICS_COMMON_HPP_

#include <chrono>
#include <cstdint>
#include <iostream>
#include <optional>
#include <tuple>

#include "timesync/chrono.hpp"

namespace openperf::packet::analyzer::statistics {

using stat_t = uint64_t;
using clock_t = openperf::timesync::chrono::realtime;

/**
 * Simple counter implementation that tracks the first/last
 * time the counter is updated.
 **/
template <typename Clock> struct timestamp_clock
{
    using timestamp = std::chrono::time_point<Clock>;
};

struct counter : timestamp_clock<clock_t>
{
    stat_t count = 0;
    timestamp first_ = timestamp::min();
    timestamp last_ = timestamp::min();

    std::optional<timestamp> first() const
    {
        return (count ? std::optional<timestamp>{first_} : std::nullopt);
    }

    std::optional<timestamp> last() const
    {
        return (count ? std::optional<timestamp>{last_} : std::nullopt);
    }

    counter& operator+=(const counter& rhs)
    {
        count += rhs.count;
        first_ = std::min(first_, rhs.first_);
        last_ = std::max(first_, rhs.last_);

        return (*this);
    }

    friend counter operator+(counter lhs, const counter& rhs)
    {
        lhs += rhs;
        return (lhs);
    }
};

template <typename Counter>
inline void
update(Counter& counter, stat_t count, typename Counter::timestamp rx) noexcept
{
    if (!counter.first()) { counter.first_ = rx; }
    counter.count += count;
    counter.last_ = rx;
}

void dump(std::ostream& os, const counter& stat, std::string_view name);

/**
 * Utility templated structs for implementing useful functions
 * for tuples of statistics
 **/

template <typename T, typename Tuple> struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...>
{};

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<has_type<StatsType, StatsTuple>::value, StatsType&>
get_counter(StatsTuple& tuple)
{
    return (std::get<StatsType>(tuple));
}

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<has_type<StatsType, StatsTuple>::value,
                           const StatsType&>
get_counter(const StatsTuple& tuple)
{
    return (std::get<StatsType>(tuple));
}

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<!has_type<StatsType, StatsTuple>::value, StatsType&>
get_counter(StatsTuple&)
{
    throw std::invalid_argument("tuple is missing requested type");
}

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<!has_type<StatsType, StatsTuple>::value,
                           const StatsType&>
get_counter(const StatsTuple&)
{
    throw std::invalid_argument("tuple is missing requested type");
}

/**
 * Run-time methods for checking if a tuple holds a particular type.
 **/

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<has_type<StatsType, StatsTuple>::value, bool>
holds_stat(const StatsTuple&)
{
    return (true);
}

template <typename StatsType, typename StatsTuple>
constexpr std::enable_if_t<!has_type<StatsType, StatsTuple>::value, bool>
holds_stat(const StatsTuple&)
{
    return (false);
}

template <bool Enable, typename LeftTuple, typename RightTuple>
constexpr auto maybe_tuple_cat(LeftTuple&& left, RightTuple&& right)
{
    if constexpr (Enable) {
        return (std::tuple_cat(std::forward<LeftTuple>(left),
                               std::forward<RightTuple>(right)));
    } else {
        return (std::forward<LeftTuple>(left));
    }
}

/**
 * Syntactic sugar for small associative data storage
 **/
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

} // namespace openperf::packet::analyzer::statistics

#endif /* _OP_ANALYZER_STATISTICS_COMMON_HPP_ */
