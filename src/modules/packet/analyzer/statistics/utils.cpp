#include "packet/analyzer/statistics/generic_flow_counters.hpp"
#include "packet/analyzer/statistics/generic_flow_digests.hpp"

namespace openperf::packet::analyzer::statistics {

template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

template <typename AssociativeArray,
          typename EnumType = decltype(
              std::declval<typename AssociativeArray::value_type>().second)>
constexpr auto to_api_string(const AssociativeArray& array, EnumType value)
{
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->first == value) return (cursor->second);
        cursor++;
    }

    return (std::string_view("none"));
}

template <typename AssociativeArray>
constexpr auto to_api_type(const AssociativeArray& array, std::string_view name)
{
    using enum_type =
        decltype(std::declval<typename AssociativeArray::value_type>().first);
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->second == name) return (cursor->first);
        cursor++;
    }

    return (enum_type{0}); /* none */
}

static constexpr auto flow_counter_pairs =
    associative_array<flow_counter_flags, std::string_view>(
        std::pair(flow_counter_flags::frame_count, "frame_count"),
        std::pair(flow_counter_flags::frame_length, "frame_length"),
        std::pair(flow_counter_flags::header, "header"),
        std::pair(flow_counter_flags::interarrival, "interarrival_time"),
        std::pair(flow_counter_flags::jitter_ipdv, "jitter_ipdv"),
        std::pair(flow_counter_flags::jitter_rfc, "jitter_rfc"),
        std::pair(flow_counter_flags::latency, "latency"),
        std::pair(flow_counter_flags::prbs, "prbs"),
        std::pair(flow_counter_flags::sequencing, "advanced_sequencing"));

static constexpr auto flow_digest_pairs =
    associative_array<flow_digest_flags, std::string_view>(
        std::pair(flow_digest_flags::frame_length, "frame_length"),
        std::pair(flow_digest_flags::interarrival, "interarrival_time"),
        std::pair(flow_digest_flags::jitter_ipdv, "jitter_ipdv"),
        std::pair(flow_digest_flags::jitter_rfc, "jitter_rfc"),
        std::pair(flow_digest_flags::latency, "latency"),
        std::pair(flow_digest_flags::sequence_run_length,
                  "sequence_run_length"));

enum flow_counter_flags to_flow_counter_flag(std::string_view name)
{
    return (to_api_type(flow_counter_pairs, name));
}

std::string_view to_name(enum flow_counter_flags flags)
{
    return (to_api_string(flow_counter_pairs, flags));
}

enum flow_digest_flags to_flow_digest_flag(std::string_view name)
{
    return (to_api_type(flow_digest_pairs, name));
}

std::string_view to_name(enum flow_digest_flags flags)
{
    return (to_api_string(flow_digest_pairs, flags));
}

} // namespace openperf::packet::analyzer::statistics
