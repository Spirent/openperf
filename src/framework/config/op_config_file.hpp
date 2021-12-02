
#ifndef _OP_CONFIG_FILE_HPP_
#define _OP_CONFIG_FILE_HPP_

#ifdef __cplusplus
#include <string_view>
#include <any>
#include <optional>
#include "tl/expected.hpp"
#include "yaml-cpp/yaml.h"
#endif /* ifdef __cplusplus */

#include "core/op_options.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Find configuration file CLI argument and, if found, set in-memory
 * configuration file name value.
 * Function will load, parse, and run some sanity checks on the file.
 *
 * @param[in] argc
 *   number of cli arguments
 * @param[in] argv
 *   array of cli strings
 *
 * @return
 *  If no errors occur return 0, non-zero otherwise.
 *
 * @note users are allowed to not specify a configuration file.
 *   In this case the function returns 0.
 */
int op_config_file_find(int argc, char* const argv[]);

/*
 * Get the value of a configuration parameter as a C-style string.
 * This is intended as a C wrapper.
 * C++ callers should use op_config_get_param() instead.
 *
 * @param[in] param
 *   path to configuration parameter.
 * @param[in] value
 *   pointer to destination char[].
 * @param[in] len
 *   maximum number of characters to copy.
 *
 * @return
 *   pointer to value if a value is found, else a pointer to NULL.
 */
char* op_config_file_get_value_str(const char* param, char* value, int len);

#ifdef __cplusplus
}
#endif

extern int framework_cli_option_handler(int opt, const char* opt_arg);

/*
 * Helper macro to create command line argument details.
 * All fields are mandatory.
 * arg_types are defined in op_config.h
 */
#define MAKE_OPT(description, long_arg, short_arg, arg_type)                   \
    {                                                                          \
        description, long_arg, short_arg, arg_type                             \
    }

/*
 * Register command-line options with the framework.
 * config_name must be globally unique.
 * init_fn is called prior to command line argument processing.
 */
#define MAKE_OPTION_DATA(config_name, init_fn, ...)                            \
    static struct op_options_data config_name##_options = {                    \
        .name = #config_name,                                                  \
        .init = init_fn,                                                       \
        .callback = framework_cli_option_handler,                              \
        .options = {                                                           \
            __VA_ARGS__{0, 0, 0, 0},                                           \
        }}

/*
 * Helper macro to connect framework configuration support to low-level
 * command line support.
 */
#define REGISTER_CLI_OPTIONS(config_name)                                      \
    REGISTER_OPTIONS(config_name##_options)

#ifdef __cplusplus
namespace openperf::config::file {

/*
 * Get configuration file name.
 *
 * @return
 *  configuration file name as passed in on the command line.
 */
std::string_view op_config_get_file_name();

/*
 * Get configuration parameter(s) for the specified path.
 * @param[in]  period-deliniated path to the requested parameter node
 *
 * @return
 *  a YAML::Node object representing configuration prameters, if any.
 *
 */
std::optional<YAML::Node> op_config_get_param(std::string_view param);

/*
 * Helper structs to map op_option_type to concrete type.
 *
 * Essentially allow users to use enum op_option_type values as
 * template arguments to op_config_get_param.
 *
 * op_config_get_param<OP_OPTION_TYPE_LONG> resolves to
 * op_config_get_param<long>.
 */
template <op_option_type T> struct op_option_type_maps;
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_NONE>
{
    using type = bool;
};
template <>
struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_CPUSET_STRING>
{
    using type = std::string;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_STRING>
{
    using type = std::string;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_HEX>
{
    using type = unsigned;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_LONG>
{
    using type = long;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_DOUBLE>
{
    using type = double;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_MAP>
{
    using type = std::map<std::string, std::string>;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_OPTIONS>
{
    using type = std::vector<std::string>;
};
template <> struct op_option_type_maps<op_option_type::OP_OPTION_TYPE_LIST>
{
    using type = std::vector<std::string>;
};

/*
 * Get a specific configuration parameter.
 * @param[in]  period-deliniated path to the requested parameter.
 *
 * @note this will throw on any type conversion error. YAML::BadConversion.

 * @return
 *  std::optional<> object that contains the requested value if it exists,
 *  otherwise empty.
 *
 */
template <typename T>
std::optional<T> op_config_get_param(std::string_view param)
{
    auto res = op_config_get_param(param);
    if (!res) { return (std::nullopt); }

    auto node = *res;
    if (node.IsNull()) { return (std::nullopt); }

    /* This can throw a YAML::BadConversion exception. */
    return (std::make_optional(node.as<T>()));
}

/*
 * Specialized version to handle values from OP_OPTION_TYPE enum.
 * Stuff like: op_config_get_param<OP_OPTION_TYPE_LONG>(path);
 *
 */
template <op_option_type T>
std::optional<typename op_option_type_maps<T>::type>
op_config_get_param(std::string_view param)
{
    return op_config_get_param<typename op_option_type_maps<T>::type>(param);
}

/*
 * Specialized version to handle op_config_get_param<YAML::Node>(path);
 *
 */
template <typename T>
typename std::enable_if_t<std::is_same<T, YAML::Node>::value,
                          std::optional<YAML::Node>>
op_config_get_param(std::string_view param)
{
    return op_config_get_param(param);
}

} // namespace openperf::config::file
#endif /* ifdef __cplusplus */

#endif /* _OP_CONFIG_FILE_HPP_ */
