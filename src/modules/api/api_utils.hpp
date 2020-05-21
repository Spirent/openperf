#ifndef _OP_API_UTILS_HPP_
#define _OP_API_UTILS_HPP_

#include <string>

#include "tl/expected.hpp"

namespace openperf::api::utils {

// Verify API module is up and running.
tl::expected<void, std::string> check_api_module_running();

tl::expected<void, std::string> check_api_path_ready(std::string_view path);

} // namespace openperf::api::utils
#endif
