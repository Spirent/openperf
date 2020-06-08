#include "packet/generator/api.hpp"

namespace openperf::packet::generator::api {

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

/**
 * String -> type mappings
 */

constexpr auto duration_type_names =
    associative_array<std::string_view, duration_type>(
        std::pair("indefinite", duration_type::indefinite),
        std::pair("frames", duration_type::frames),
        std::pair("seconds", duration_type::seconds));

constexpr auto layer_type_names =
    associative_array<std::string_view, layer_type>(
        std::pair("ethernet", layer_type::ethernet),
        std::pair("ip", layer_type::ip),
        std::pair("protocol", layer_type::protocol),
        std::pair("payload", layer_type::payload));

constexpr auto load_type_names = associative_array<std::string_view, load_type>(
    std::pair("frames", load_type::frames),
    std::pair("octets", load_type::octets));

constexpr auto mux_type_names = associative_array<std::string_view, mux_type>(
    std::pair("zip", mux_type::zip),
    std::pair("cartesian", mux_type::cartesian));

constexpr auto order_type_names =
    associative_array<std::string_view, order_type>(
        std::pair("round-robin", order_type::round_robin),
        std::pair("sequential", order_type::sequential));

constexpr auto period_type_names =
    associative_array<std::string_view, period_type>(
        std::pair("hour", period_type::hours),
        std::pair("hours", period_type::hours),
        std::pair("millisecond", period_type::milliseconds),
        std::pair("milliseconds", period_type::milliseconds),
        std::pair("minute", period_type::minutes),
        std::pair("minutes", period_type::minutes),
        std::pair("second", period_type::seconds),
        std::pair("seconds", period_type::seconds));

constexpr auto signature_latency_type_names =
    associative_array<std::string_view, signature_latency_type>(
        std::pair("start_of_frame", signature_latency_type::start_of_frame),
        std::pair("end_of_frame", signature_latency_type::end_of_frame));

/**
 * String -> type functions
 */

layer_type to_layer_type(std::string_view name)
{
    return (to_api_type(layer_type_names, name));
}

load_type to_load_type(std::string_view name)
{
    return (to_api_type(load_type_names, name));
}

mux_type to_mux_type(std::string_view name)
{
    return (to_api_type(mux_type_names, name));
}

order_type to_order_type(std::string_view name)
{
    return (to_api_type(order_type_names, name));
}

period_type to_period_type(std::string_view name)
{
    return (to_api_type(period_type_names, name));
}

signature_latency_type to_signature_latency_type(std::string_view name)
{
    return (to_api_type(signature_latency_type_names, name));
}

/**
 * Type -> string functions
 */

std::string to_duration_string(enum duration_type duration)
{
    return (std::string(to_api_string(duration_type_names, duration)));
}

} // namespace openperf::packet::generator::api
