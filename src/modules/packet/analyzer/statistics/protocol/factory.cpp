#include <functional>
#include <vector>

#include "packet/analyzer/statistics/generic_protocol_counters.hpp"

namespace openperf::packet::analyzer::statistics {

namespace protocol {

constexpr auto to_value(openperf::utils::bit_flags<protocol_flags> flags)
{
    return (flags.value);
}

template <size_t FlagValue> auto make_counter_tuple()
{
    /* All protocol counters are independent */
    auto t1 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::ethernet))>(
        std::tuple<>{}, std::tuple<ethernet>{});

    auto t2 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::ip))>(t1, std::tuple<ip>{});

    auto t3 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::protocol))>(
        t2, std::tuple<protocol>{});

    auto t4 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::tunnel))>(t3,
                                                       std::tuple<tunnel>{});

    auto t5 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::inner_ethernet))>(
        t4, std::tuple<inner_ethernet>{});

    auto t6 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::inner_ip))>(
        t5, std::tuple<inner_ip>{});

    auto t7 = maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(protocol_flags::inner_protocol))>(
        t6, std::tuple<inner_protocol>{});

    return (t7);
}

template <size_t I> constexpr auto make_counters_constructor()
{
    return (
        []() { return (generic_protocol_counters(make_counter_tuple<I>())); });
}

template <size_t... I>
auto make_counters_constructor_index(std::index_sequence<I...>)
{
    auto m = std::vector<std::function<generic_protocol_counters()>>{};
    (m.emplace_back(make_counters_constructor<I>()), ...);
    return (m);
}

} // namespace protocol

/*
 * Since our possible flag values range from [0, all_protocol_counters],
 * we can use an integer sequence to handle generating every value without
 * having to manually enumerate every one.
 * Note: we have to add one because make_index_sequence generates values from
 * 0 to n-1.
 */
generic_protocol_counters
make_counters(openperf::utils::bit_flags<protocol_flags> flags)
{
    const static auto constructors = protocol::make_counters_constructor_index(
        std::make_index_sequence<protocol::to_value(all_protocol_counters)
                                 + 1>{});
    return (constructors[protocol::to_value(flags)]());
}

} // namespace openperf::packet::analyzer::statistics
