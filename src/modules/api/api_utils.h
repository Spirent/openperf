#ifndef _ICP_API_UTILS_H_
#define _ICP_API_UTILS_H_

namespace icp::api::utils {
// Verify API module is up and running.
// @returns 0 if API module is up and responding to requests, -1 otherwise.
int check_api_module_running();
}  // namespace icp::api::utils
#endif
