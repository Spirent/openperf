#include "icp_config_utils.h"
#include <regex>

namespace icp::config {

// Regular expression used to validate resource IDs.
static const std::string_view id_regex = "^[a-z0-9-]+$";

tl::expected<void, std::string> icp_config_validate_id_string(std::string_view id)
{
    if (id.empty()) { return (tl::make_unexpected("Resource IDs must be non-empty strings.")); }

    auto regex = std::regex(id_regex.data(), std::regex::extended);

    if (!std::regex_match(id.data(), regex)) {
        return (tl::make_unexpected("Resource ID " + std::string(id)
                                    + " is not a valid identifier. Identifiers may only contain "
                                      "lower-case letters, numbers, and hyphens."));
    }

    return {};
}

}  // namespace icp::config
