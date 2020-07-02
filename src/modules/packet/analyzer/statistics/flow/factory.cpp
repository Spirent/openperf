#include <functional>
#include <tuple>
#include <type_traits>
#include <vector>

#include "packet/analyzer/statistics/generic_flow_counters.hpp"

namespace openperf::packet::analyzer::statistics {

namespace flow {

/* Equivalent to std::type_identity from c++20 */
template <class T> struct type_identity
{
    using type = T;
};

template <typename T, typename... Ts>
struct filter_duplicates : type_identity<T>
{};

template <typename... Ts, typename U, typename... Us>
struct filter_duplicates<std::tuple<Ts...>, std::tuple<U, Us...>>
    : std::conditional_t<
          std::disjunction_v<std::is_same<U, Ts>...>,
          filter_duplicates<std::tuple<Ts...>, std::tuple<Us...>>,
          filter_duplicates<std::tuple<Ts..., U>, std::tuple<Us...>>>
{};

template <typename... Ts>
using unique_tuple_t = typename filter_duplicates<std::tuple<>, Ts...>::type;

constexpr auto to_value(openperf::utils::bit_flags<flow_flags> flags)
{
    return (flags.value);
}

template <int FlagValue> auto make_counter_tuple()
{
    /* Basic stats are always required */
    auto t1 = std::tuple<frame_counter>{};

    auto t2 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::interarrival))>(
        t1, std::tuple<interarrival>{});

    auto t3 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::frame_length))>(
        t2, std::tuple<frame_length>{});

    auto t4 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::sequencing))>(
        t3, std::tuple<sequencing>{});

    auto t5 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::latency))>(t4, std::tuple<latency>{});

    /* RFC jitter stats require latency stats as well */
    auto t6 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::jitter_rfc))>(
        t5, std::tuple<latency, jitter_rfc>{});

    /* IPDV jitter stats require both latency and sequencing */
    auto t7 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::jitter_ipdv))>(
        t6, std::tuple<sequencing, latency, jitter_ipdv>{});

    auto t8 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::prbs))>(t7, std::tuple<prbs>{});

    auto t9 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_flags::header))>(t8, std::tuple<header>{});

    /*
     * Since we might have added duplicate results to our tuple, filter out any
     * dups.
     */
    return (unique_tuple_t<decltype(t9)>{});
}

template <size_t I> constexpr auto make_counters_constructor()
{
    return ([]() { return (generic_flow_counters(make_counter_tuple<I>())); });
}

template <size_t... I>
auto make_counters_constructor_index(std::index_sequence<I...>)
{
    auto m = std::vector<std::function<generic_flow_counters()>>{};
    (m.emplace_back(make_counters_constructor<I>()), ...);
    return (m);
}

} // namespace flow

/*
 * Since our possible flag values range from [0, all_flow_counters],
 * we can use an integer sequence to handle generating every value without
 * having to manually enumerate every one.
 * Note: we have to add one because make_index_sequence generates values from
 * 0 to n-1.
 */
generic_flow_counters
make_counters(openperf::utils::bit_flags<flow_flags> flags)
{
    const static auto constructors = flow::make_counters_constructor_index(
        std::make_index_sequence<flow::to_value(all_flow_counters) + 1>{});
    return (constructors[flow::to_value(flags)]());
}

} // namespace openperf::packet::analyzer::statistics
