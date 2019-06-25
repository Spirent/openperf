
#ifndef _ICP_CONFIG_FILE_H_
#define _ICP_CONFIG_FILE_H_

#ifdef __cplusplus
#include <string_view>
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

std::string_view icp_get_config_file_name();

tl::expected<YAML::Node, std::string> icp_get_module_config(std::string_view module_name);

}  // namespace icp::config::file
#endif /* ifdef __cplusplus */

#endif /* _ICP_CONFIG_FILE_H_ */
