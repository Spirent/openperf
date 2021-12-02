#ifndef _OP_CPU_RESULTS_HPP_
#define _OP_CPU_RESULTS_HPP_
#include <cassert>

#include <array>
#include <cstdint>
#include <numeric>
#include <optional>
#include <variant>

#include "cpu/generator/load_types.hpp"
#include "cpu/generator/system_stats.hpp"

namespace openperf::cpu {

template <typename InstructionSet, typename DataType> struct target_stats_impl
{
    int64_t operations;

    target_stats_impl& operator+=(const target_stats_impl& rhs)
    {
        operations += rhs.operations;
        return (*this);
    }

    friend target_stats_impl operator+(target_stats_impl lhs,
                                       const target_stats_impl& rhs)
    {
        lhs += rhs;
        return (lhs);
    }
};

template <typename... Pairs>
std::variant<target_stats_impl<typename Pairs::first_type,
                               typename Pairs::second_type>...>
    as_target_stats_variant(type_list<Pairs...>);

using target_stats = decltype(as_target_stats_variant(load_types{}));

template <typename... Types>
constexpr auto variant_array(std::variant<Types...>)
    -> std::array<std::variant<Types...>,
                  std::variant_size_v<std::variant<Types...>>>
{
    return {{Types{}...}};
}

template <typename T> struct range
{
    T first;
    T last;

    range& operator+=(const range& other)
    {
        first = std::min(first, other.first);
        last = std::max(last, other.last);
        return (*this);
    }
};

template <typename T> range<T> operator+(range<T> lhs, const range<T>& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Clock> struct core_stats
{
    using timestamp = typename Clock::time_point;
    using duration = typename Clock::duration;

    /*
     * XXX: The expectation is that steal time is accumulated since boot.
     * However, we only want to report the steal time that occurs over a test
     * duration, hence we need to store a range for steal time.
     */
    range<timestamp> time_;
    range<duration> steal_;
    duration system;
    duration target;
    duration user;
    std::array<target_stats, std::variant_size_v<target_stats>> targets;

    core_stats(timestamp timestamp_first,
               timestamp timestamp_last,
               duration steal_first,
               duration steal_last)
        : time_({timestamp_first, timestamp_last})
        , steal_({steal_first, steal_last})
        , system(duration::zero())
        , target(duration::zero())
        , user(duration::zero())
    {
        targets = variant_array(target_stats{});
    }

    core_stats(timestamp now, duration steal)
        : core_stats(now, now, steal, steal)
    {}

    core_stats()
        : core_stats(timestamp::max(),
                     timestamp::min(),
                     duration::max(),
                     duration::min())
    {}

    core_stats(const core_stats& other)
        : time_(other.time_)
        , steal_(other.steal_)
        , system(other.system)
        , target(other.target)
        , user(other.user)
        , targets(other.targets)
    {}

    core_stats operator=(const core_stats& other)
    {
        if (this != &other) {
            time_ = other.time_;
            steal_ = other.steal_;
            system = other.system;
            target = other.target;
            user = other.user;
            std::copy(std::begin(other.targets),
                      std::end(other.targets),
                      std::begin(targets));
        }
        return (*this);
    }

    duration available() const
    {
        if (!first() || !last()) { return (duration::zero()); }

        return (time_.last - time_.first);
    }

    duration error() const { return (utilization() - target); }

    duration utilization() const { return (system + user); }

    duration steal() const { return (steal_.last - steal_.first); }

    std::optional<timestamp> first() const
    {
        return (utilization() > duration::zero()
                    ? std::optional<timestamp>{time_.first}
                    : std::nullopt);
    }

    std::optional<timestamp> last() const
    {
        return (utilization() > duration::zero()
                    ? std::optional<timestamp>{time_.last}
                    : std::nullopt);
    }

    template <typename Container, size_t... N>
    void
    plus_equals(Container& lhs, const Container& rhs, std::index_sequence<N...>)
    {
        ((std::get<N>(lhs[N]) += std::get<N>(rhs[N])), ...);
    }

    core_stats& operator+=(const core_stats& rhs)
    {
        time_ += rhs.time_;
        steal_ += rhs.steal_;
        system += rhs.system;
        target += rhs.target;
        user += rhs.user;

        plus_equals(
            targets,
            rhs.targets,
            std::make_index_sequence<std::variant_size_v<target_stats>>{});

        assert(first() <= last());
        return (*this);
    }
};

template <typename Clock>
inline core_stats<Clock> operator+(core_stats<Clock> lhs,
                                   const core_stats<Clock>& rhs)
{
    lhs += rhs;
    return (lhs);
}

template <typename Clock>
inline void init_stats(core_stats<Clock>& stats,
                       typename Clock::time_point ts,
                       typename Clock::duration steal)
{
    stats.time_.first = ts;
    stats.time_.last = ts;
    stats.steal_.first = steal;
    stats.steal_.last = steal;
}

template <typename Clock>
inline core_stats<Clock> sum_stats(const std::vector<core_stats<Clock>>& shards)
{
    auto sum = std::accumulate(std::begin(shards),
                               std::end(shards),
                               core_stats<Clock>{},
                               std::plus<>{});
    return (sum);
}

} // namespace openperf::cpu

#endif /* _OP_CPU_RESULTS_HPP_ */
