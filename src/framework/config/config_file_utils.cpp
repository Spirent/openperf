#include "config_file_utils.h"
#include <tuple>
#include <cstdlib>

namespace icp::config::file {

bool icp_config_is_number(const std::string_view value)
{
    if (value.empty()) { return false; }

    char *endptr;

    // Is it an integer?
    strtoll(value.data(), &endptr, 0);
    if ((endptr - value.data()) == value.length()) { return true; }

    // How about a double?
    strtod(value.data(), &endptr);
    if ((endptr - value.data()) == value.length()) { return true; }

    return false;
}

std::tuple<std::string_view, std::string_view> icp_config_split_path_id(std::string_view path_id)
{
    size_t last_slash_location = path_id.find_last_of('/');
    if (last_slash_location == std::string_view::npos) {
        // No slash. Not a valid /path/id string.
        return std::tuple(std::string_view(), std::string_view());
    }

    if (last_slash_location == 0) { return std::tuple(path_id, std::string_view()); }

    return std::tuple(path_id.substr(0, last_slash_location),
                      path_id.substr(last_slash_location + 1));
}

}  // namespace icp::config::file
