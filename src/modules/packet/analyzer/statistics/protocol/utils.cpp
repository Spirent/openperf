#include "packet/analyzer/statistics/generic_protocol_counters.hpp"

namespace openperf::packet::analyzer::statistics {

static constexpr auto flag_names =
    associative_array<protocol_flags, std::string_view>(
        std::pair(protocol_flags::ethernet, "ethernet"),
        std::pair(protocol_flags::ip, "ip"),
        std::pair(protocol_flags::protocol, "protocol"),
        std::pair(protocol_flags::tunnel, "tunnel"),
        std::pair(protocol_flags::inner_ethernet, "inner_ethernet"),
        std::pair(protocol_flags::inner_ip, "inner_ip"),
        std::pair(protocol_flags::inner_protocol, "inner_protocol"));

enum protocol_flags to_protocol_flag(std::string_view name)
{
    auto cursor = std::begin(flag_names), end = std::end(flag_names);
    while (cursor != end) {
        if (cursor->second == name) return (cursor->first);
        cursor++;
    }

    return (protocol_flags::none);
}

std::string_view to_name(enum protocol_flags flag)
{
    auto cursor = std::begin(flag_names), end = std::end(flag_names);
    while (cursor != end) {
        if (cursor->first == flag) return (cursor->second);
        cursor++;
    }

    return ("none");
}

} // namespace openperf::packet::analyzer::statistics
