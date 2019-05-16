#ifndef _ICP_API_UTILS_H_
#define _ICP_API_UTILS_H_

#include "tl/expected.hpp"

namespace icp::api::utils {

// Verify API module is up and running.
tl::expected<void, std::string> check_api_module_running();

}  // namespace icp::api::utils
#endif
