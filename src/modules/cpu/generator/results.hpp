#ifndef _OP_CPU_RESULTS_HPP_
#define _OP_CPU_RESULTS_HPP_
#include <cassert>

#include <array>
#include <cstdint>
#include <optional>
#include <variant>

#include "cpu/generator/load_types.hpp"

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

template <typename Clock> struct core_stats
{
    using timestamp = typename Clock::time_point;
    using duration = typename Clock::duration;

    timestamp first_;
    timestamp last_;
    duration target;
    duration system;
    duration user;
    duration steal;
    std::array<target_stats, std::variant_size_v<target_stats>> targets;

    core_stats(timestamp first, timestamp last)
        : first_(first)
        , last_(last)
        , target(duration::zero())
        , system(duration::zero())
        , user(duration::zero())
        , steal(duration::zero())
    {
        targets = variant_array(target_stats{});
    }

    core_stats(timestamp now)
        : core_stats(now, now)
    {}

    core_stats()
        : core_stats(timestamp::max(), timestamp::min())
    {}

    core_stats(const core_stats& other)
        : first_(other.first_)
        , last_(other.last_)
        , target(other.target)
        , system(other.system)
        , user(other.user)
        , steal(other.steal)
        , targets(other.targets)
    {}

    core_stats operator=(const core_stats& other)
    {
        if (this != &other) {
            first_ = other.first_;
            last_ = other.last_;
            target = other.target;
            system = other.system;
            user = other.user;
            steal = other.steal;
            std::copy(std::begin(other.targets),
                      std::end(other.targets),
                      std::begin(targets));
        }
        return (*this);
    }

    duration available() const
    {
        if (!first() || !last()) { return (duration::zero()); }

        return (last_ - first_);
    }

    duration error() const { return (utilization() - target); }

    duration utilization() const { return (system + user); }

    std::optional<timestamp> first() const
    {
        return (utilization() > duration::zero()
                    ? std::optional<timestamp>{first_}
                    : std::nullopt);
    }

    std::optional<timestamp> last() const
    {
        return (utilization() > duration::zero()
                    ? std::optional<timestamp>{last_}
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
        first_ = std::min(first_, rhs.first_);
        last_ = std::max(last_, rhs.last_);
        target += rhs.target;
        system += rhs.system;
        user += rhs.user;
        steal += rhs.steal;

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

} // namespace openperf::cpu

#endif /* _OP_CPU_RESULTS_HPP_ */
