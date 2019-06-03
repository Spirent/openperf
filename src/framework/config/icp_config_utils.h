#ifndef _ICP_CONFIG_UTILS_H_
#define _ICP_CONFIG_UTILS_H_

#include <string>
#include "tl/expected.hpp"

namespace icp::config {

// Function to validate if a supplied resource ID string is valid.
tl::expected<void, std::string> icp_config_validate_id_string(std::string_view id);

}  // namespace icp::config

#endif
