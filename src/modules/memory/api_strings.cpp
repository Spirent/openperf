#include "memory/api.hpp"
#include "utils/associative_array.hpp"

namespace openperf::memory::api {

constexpr auto pattern_type_names =
    utils::associative_array<std::string_view, pattern_type>(
        std::pair("random", pattern_type::random),
        std::pair("sequential", pattern_type::sequential),
        std::pair("reverse", pattern_type::reverse));

pattern_type to_pattern_type(std::string_view name)
{
    return (utils::key_to_value(pattern_type_names, name)
                .value_or(pattern_type::none));
}

std::string to_string(pattern_type type)
{
    return (std::string(
        utils::value_to_key(pattern_type_names, type).value_or("unknown")));
}

} // namespace openperf::memory::api
