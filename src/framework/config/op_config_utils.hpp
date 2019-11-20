#ifndef _OP_CONFIG_UTILS_HPP_
#define _OP_CONFIG_UTILS_HPP_

#include <string>
#include "tl/expected.hpp"

namespace openperf::config {

// Function to validate if a supplied resource ID string is valid.
tl::expected<void, std::string> op_config_validate_id_string(std::string_view id);

}  // namespace openperf::config

#endif
