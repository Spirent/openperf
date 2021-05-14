#ifndef _OP_PACKET_STATISTICS_UTILS_HPP_
#define _OP_PACKET_STATISTICS_UTILS_HPP_

#include <tuple>
#include <stdexcept>

namespace openperf::packet::statistics {

/**
 * Utility templated structs for implementing useful functions
 * for tuples of statistics
 **/

template <typename T, typename Tuple> struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...>
{};

template <typename T, typename U>
inline constexpr bool has_type_v = has_type<T, U>::value;

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

} // namespace openperf::packet::statistics

#endif /* _OP_PACKET_STATISTICS_UTILS_HPP_ */
