#include "packet/statistics/generic_protocol_counters.hpp"

namespace openperf::packet::statistics {

/**
 * Syntactic sugar for small associative data storage
 **/
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

static constexpr auto flag_names =
    associative_array<protocol_flags, std::string_view>(
        std::pair(protocol_flags::ethernet, "ethernet"),
        std::pair(protocol_flags::ip, "ip"),
        std::pair(protocol_flags::transport, "transport"),
        std::pair(protocol_flags::tunnel, "tunnel"),
        std::pair(protocol_flags::inner_ethernet, "inner_ethernet"),
        std::pair(protocol_flags::inner_ip, "inner_ip"),
        std::pair(protocol_flags::inner_transport, "inner_transport"));

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

} // namespace openperf::packet::statistics
