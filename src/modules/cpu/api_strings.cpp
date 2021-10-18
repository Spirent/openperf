#include "cpu/api.hpp"
#include "utils/associative_array.hpp"

namespace openperf::cpu::api {

/*
 * String -> type mappings
 */

constexpr auto method_type_names =
    utils::associative_array<std::string_view, method_type>(
        std::pair("system", method_type::system),
        std::pair("cores", method_type::cores));

constexpr auto instruction_type_names =
    utils::associative_array<std::string_view, instruction_type>(
        std::pair("scalar", instruction_type::scalar),
        std::pair("sse2", instruction_type::sse2),
        std::pair("sse4", instruction_type::sse4),
        std::pair("avx", instruction_type::avx),
        std::pair("avx2", instruction_type::avx2),
        std::pair("avx512", instruction_type::avx512),
        std::pair("neon", instruction_type::neon));

constexpr auto data_type_names =
    utils::associative_array<std::string_view, data_type>(
        std::pair("int32", data_type::int32),
        std::pair("int64", data_type::int64),
        std::pair("float32", data_type::float32),
        std::pair("float64", data_type::float64));

/*
 * String -> type functions
 */

template <typename AssociativeArray>
constexpr auto to_api_type(const AssociativeArray& array, std::string_view name)
{
    using enum_type =
        decltype(std::declval<typename AssociativeArray::value_type>().second);

    if (auto result = utils::key_to_value(array, name)) {
        return (result.value());
    }

    return (enum_type{0});
}

method_type to_method_type(std::string_view name)
{
    return (to_api_type(method_type_names, name));
}

instruction_type to_instruction_type(std::string_view name)
{
    return (to_api_type(instruction_type_names, name));
}

data_type to_data_type(std::string_view name)
{
    return (to_api_type(data_type_names, name));
}

/*
 * Type -> string functions
 */
template <typename AssociativeArray, typename T>
constexpr std::string_view to_string(const AssociativeArray& array, T value)
{
    if (auto result = utils::value_to_key(array, value)) {
        return (result.value());
    }

    return ("unknown");
}

std::string to_string(method_type type)
{
    return (std::string(to_string(method_type_names, type)));
}

std::string to_string(instruction_type type)
{
    return (std::string(to_string(instruction_type_names, type)));
}

std::string to_string(data_type type)
{
    return (std::string(to_string(data_type_names, type)));
}

} // namespace openperf::cpu::api
