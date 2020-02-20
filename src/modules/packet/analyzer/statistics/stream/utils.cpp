#include "packet/analyzer/statistics/generic_stream_counters.hpp"

namespace openperf::packet::analyzer::statistics {

static constexpr auto flag_names =
    associative_array<stream_flags, std::string_view>(
        std::pair(stream_flags::frame_count, "frame_count"),
        std::pair(stream_flags::interarrival, "interarrival_time"),
        std::pair(stream_flags::frame_length, "frame_length"),
        std::pair(stream_flags::sequencing, "advanced_sequencing"),
        std::pair(stream_flags::latency, "latency"),
        std::pair(stream_flags::jitter_ipdv, "jitter_ipdv"),
        std::pair(stream_flags::jitter_rfc, "jitter_rfc"),
        std::pair(stream_flags::prbs, "prbs"));

enum stream_flags to_stream_flag(std::string_view name)
{
    auto cursor = std::begin(flag_names), end = std::end(flag_names);
    while (cursor != end) {
        if (cursor->second == name) return (cursor->first);
        cursor++;
    }

    return (stream_flags::none);
}

std::string_view to_name(enum stream_flags flag)
{
    auto cursor = std::begin(flag_names), end = std::end(flag_names);
    while (cursor != end) {
        if (cursor->first == flag) return (cursor->second);
        cursor++;
    }

    return ("none");
}

} // namespace openperf::packet::analyzer::statistics
