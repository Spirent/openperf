#ifndef _API_CONFIG_FILE_RESOURCES_HPP_
#define _API_CONFIG_FILE_RESOURCES_HPP_

#include "tl/expected.hpp"

namespace openperf::api::config {

// Process 'resources:' section of a configuration file.
// Function takes entries in the 'resources:' section and POSTs them to the REST
// API. On encountering an error, function will stop processing and return
// immediately.
tl::expected<void, std::string> op_config_file_process_resources();

} // namespace openperf::api::config

#endif /* _API_CONFIG_FILE_RESOURCES_HPP_ */
