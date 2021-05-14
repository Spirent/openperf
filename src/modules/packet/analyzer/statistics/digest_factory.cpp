#include "packet/analyzer/statistics/generic_flow_digests.hpp"

namespace openperf::packet::analyzer::statistics {

namespace detail {

constexpr auto to_value(openperf::utils::bit_flags<flow_digest_flags> flags)
{
    return (flags.value);
}

template <int FlagValue> auto make_flow_digests_tuple()
{
    auto t0 = std::tuple<>{};

    auto t1 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::frame_length))>(
        t0, std::tuple<flow::digest::frame_length>{});

    auto t2 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::interarrival))>(
        t1, std::tuple<flow::digest::interarrival>{});

    auto t3 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::jitter_ipdv))>(
        t2, std::tuple<flow::digest::jitter_ipdv>{});

    auto t4 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::jitter_rfc))>(
        t3, std::tuple<flow::digest::jitter_rfc>{});

    auto t5 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::latency))>(
        t4, std::tuple<flow::digest::latency>{});

    auto t6 = packet::statistics::maybe_tuple_cat<static_cast<bool>(
        FlagValue & to_value(flow_digest_flags::sequence_run_length))>(
        t5, std::tuple<flow::digest::sequence_run_length>{});

    return (t6);
}

template <size_t I> constexpr auto make_flow_digests_constructor()
{
    return (
        []() { return (generic_flow_digests(make_flow_digests_tuple<I>())); });
}

template <size_t... I>
auto make_flow_digests_constructor_index(std::index_sequence<I...>)
{
    auto constructors = std::vector<std::function<generic_flow_digests()>>{};
    (constructors.emplace_back(make_flow_digests_constructor<I>()), ...);
    return (constructors);
}

} // namespace detail

generic_flow_digests
make_flow_digests(openperf::utils::bit_flags<flow_digest_flags> flags)
{
    const static auto constructors =
        detail::make_flow_digests_constructor_index(
            std::make_index_sequence<detail::to_value(all_flow_digests) + 1>{});
    return (constructors[detail::to_value(flags)]());
}

} // namespace openperf::packet::analyzer::statistics
