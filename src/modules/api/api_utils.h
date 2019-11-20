#ifndef _OP_API_UTILS_H_
#define _OP_API_UTILS_H_

#include "tl/expected.hpp"

namespace openperf::api::utils {

// Verify API module is up and running.
tl::expected<void, std::string> check_api_module_running();

}  // namespace openperf::api::utils
#endif
