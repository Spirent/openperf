
#ifndef _ICP_CONFIG_FILE_H_
#define _ICP_CONFIG_FILE_H_

#include <string_view>
#include "tl/expected.hpp"
#include "yaml-cpp/yaml.h"

namespace icp::config::file {

std::string_view icp_get_config_file_name();

tl::expected<YAML::Node, std::string> icp_get_module_config(std::string_view module_name);

}  // namespace icp::config::file

#endif /* _ICP_CONFIG_FILE_H_ */
