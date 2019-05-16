#ifndef _API_CONFIG_FILE_RESOURCES_H_
#define _API_CONFIG_FILE_RESOURCES_H_

#include "tl/expected.hpp"

namespace icp::api::config {

tl::expected<void, std::string> icp_config_file_process_resources();

}  // namespace icp::api::config

#endif /* _API_CONFIG_FILE_RESOURCES_H_ */
