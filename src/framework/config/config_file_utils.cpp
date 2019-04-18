#include "config_file_utils.h"
#include <tuple>
#include <cstdlib>

namespace icp::config::file {

bool icp_config_is_number(const std::string &value)
{
    if (value.empty()) { return false; }

    char *endptr;

    // Is it an integer?
    strtoll(value.c_str(), &endptr, 0);
    if ((endptr - value.c_str()) == value.length()) { return true; }

    // How about a double?
    strtod(value.c_str(), &endptr);
    if ((endptr - value.c_str()) == value.length()) { return true; }

    return false;
}

std::tuple<std::string, std::string> icp_config_split_path_id(const std::string &path_id)
{
    size_t last_slash_location = path_id.find_last_of('/');
    if (last_slash_location == std::string::npos) {
        // No slash. Not a valid /path/id string.
        return std::tuple(std::string(), std::string());
    }

    if (last_slash_location == 0) { return std::tuple(path_id, std::string()); }

    return std::tuple(path_id.substr(0, last_slash_location),
                      path_id.substr(last_slash_location + 1));
}

}  // namespace icp::config::file
