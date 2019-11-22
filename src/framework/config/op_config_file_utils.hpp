#ifndef _OP_CONFIG_FILE_UTILS_HPP_
#define _OP_CONFIG_FILE_UTILS_HPP_

#include <string>
#include "tl/expected.hpp"
#include "yaml-cpp/yaml.h"

namespace openperf::config::file {

// Function to check if a given input string is
// either a decimal or integer number.
// Not an exhaustive checker (only supports base-10).
bool op_config_is_number(std::string_view value);

// Function to split a resource path from an identifier.
// In OpenPerf it is assumed that /<path>/<id> in a configuration file maps to
// a REST endpoint at /<path> and a user-defined identifier, <id>.
// <path> may also include multiple forward slashes.
// Function requires a null-terminated string. Otherwise behavior is undefined.
std::tuple<std::string_view, std::string_view>
op_config_split_path_id(std::string_view path_id);

// Function to convert YAML to JSON.
// Used to adapt config file information (written in YAML)
// to something the REST API can consume (JSON).
tl::expected<std::string, std::string>
op_config_yaml_to_json(const YAML::Node& yaml_src);

} // namespace openperf::config::file

#endif /*_OP_CONFIG_FILE_UTILS_HPP_*/
