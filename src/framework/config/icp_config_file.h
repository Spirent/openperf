
#ifndef _ICP_CONFIG_FILE_H_
#define _ICP_CONFIG_FILE_H_

#ifdef __cplusplus
#include <string_view>
#include <any>
#include "tl/expected.hpp"
#include "yaml-cpp/yaml.h"
#endif /* ifdef __cplusplus */

#include "core/icp_options.h"

extern int module_option_handler(int opt, const char *opt_arg);

#define MAKE_OPT(description, long_arg, short_arg, has_param, arg_type) \
    {                                                                   \
     description, long_arg, short_arg, has_param, arg_type              \
         }                                                              \

#define MAKE_OPTION_DATA(module_name, init_fn, ...)                     \
    static struct icp_options_data module_name##_options =              \
        {                                                               \
         .name = #module_name,                                          \
         .init = init_fn,                                               \
         .callback = module_option_handler,                             \
         .options = {                                                   \
                     __VA_ARGS__                                        \
                     {0, 0, 0, 0, 0},                                   \
                     }                                                  \
        }

#define REGISTER_MODULE_OPTIONS(module_name) \
    REGISTER_OPTIONS(module_name##_options)

#ifdef __cplusplus
namespace icp::config::file {

/*
 * Get configuration file name.
 *
 * @return
 *  configuration file name as passed in on the command line.
 */
std::string_view icp_config_get_file_name();

/*
 * Get configuration parameters for the specified module.
 * @param[in]  module_name name of the module to get parameters for.
 *
 * @return
 *  a YAML::Node object representing configuration prameters, if any.
 *
 */
tl::expected<YAML::Node, std::string> icp_config_get_module_params(std::string_view module_name);

/*
 * Get a specific module configuration parameter.
 * @param[in]  period-deliniated path to the requested parameter.
 *
 * @return
 *  a expected<> object with a (possibly) empty std::any or, on error, a string with details.
 *
 */
tl::expected<std::any, std::string> icp_config_get_module_param(std::string_view param);

}  // namespace icp::config::file
#endif /* ifdef __cplusplus */

#endif /* _ICP_CONFIG_FILE_H_ */
