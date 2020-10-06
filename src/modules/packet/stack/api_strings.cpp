#include <string>

#include "packet/stack/api.hpp"

namespace openperf::packet::stack::api {

/*
 * Provide some mappings between REST API strings and strict
 * types. Since each set of strings is relatively small, we
 * just use a static array of pairs instead of embracing the
 * glory and overhead of static maps.
 */
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

template <typename AssociativeArray>
constexpr auto to_api_type(const AssociativeArray& array, std::string_view name)
{
    using enum_type =
        decltype(std::declval<typename AssociativeArray::value_type>().second);
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->first == name) return (cursor->second);
        cursor++;
    }

    return (enum_type{0}); /* none */
}

template <typename AssociativeArray,
          typename EnumType = decltype(
              std::declval<typename AssociativeArray::value_type>().second)>
constexpr auto to_api_string(const AssociativeArray& array, EnumType value)
{
    auto cursor = std::begin(array), end = std::end(array);
    while (cursor != end) {
        if (cursor->second == value) return (cursor->first);
        cursor++;
    }

    return (std::string_view("unknown"));
}

constexpr auto intf_filter_type_names =
    associative_array<std::string_view, intf_filter_type>(
        std::pair("port_id", intf_filter_type::port_id),
        std::pair("eth_mac_address", intf_filter_type::eth_mac_address),
        std::pair("ipv4_address", intf_filter_type::ipv4_address),
        std::pair("ipv6_address", intf_filter_type::ipv6_address));

std::string to_string(intf_filter_type type)
{
    return (std::string(to_api_string(intf_filter_type_names, type)));
}

} // namespace openperf::packet::stack::api
