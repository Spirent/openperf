#ifndef _ICP_CONFIG_FILE_UTILS_H_
#define _ICP_CONFIG_FILE_UTILS_H_

#include <string>

namespace icp::config::file {

// Function to check if a given input string is
// either a decimal or integer number.
// Not an exhaustive checker (only supports base-10).
bool icp_config_is_number(const std::string &value);

// Function to split a resource path from an identifier.
// In Inception it is assumed that /<path>/<id> in a configuration file maps to
// a REST endpoint at /<path> and a user-defined identifier, <id>.
// <path> may also include multiple forward slashes.
std::tuple<std::string, std::string> icp_config_split_path_id(const std::string &path_id);

}  // namespace icp::config::file

#endif /*_ICP_CONFIG_FILE_UTILS_H_*/
