#include "core/icp_common.h"
#include "icp_config_file.h"
#include <iostream>
#include <unistd.h>
#include "yaml-cpp/yaml.h"
#include <string.h>

namespace icp::config::file {

using namespace std;

static string config_file_name;

std::string_view icp_get_config_file_name()
{
    return config_file_name;
}

extern "C" {
int config_file_option_handler(int opt, const char *opt_arg)
{
    if (opt != 'c' || opt_arg == nullptr) { return (EINVAL); }

    config_file_name = opt_arg;

    // Make sure the file exists and is readable.
    if (access(opt_arg, R_OK) == -1) {
        std::cerr << "Error (" << strerror(errno)
                  << ") while attempting to access config file: " << opt_arg << std::endl;
        return (errno);
    }

    // This will do an initial parse. yaml-cpp throws exceptions when the parser runs into invalid
    // YAML.
    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception &e) {
        std::cerr << "Error parsing configuration file: " << e.what() << std::endl;
        return (EINVAL);
    }

    // Validate there are the two required top-level nodes "core" and "resources".
    // Also check for the optional "modules" resource, and that no other top-level nodes exist.
    if ((root_node.size() == 2) && root_node["core"] && root_node["resources"]) {
        return (0);
    } else if ((root_node.size() == 3) && root_node["core"] && root_node["resources"]
               && root_node["modules"]) {
        return (0);
    }

    std::cerr << "Configuration file must only contain top-level sections \"core:\" and "
                 "\"resources\", and, optionally, \"modules:\""
              << std::endl;

    return (EINVAL);
}

}  // extern "C"

}  // namespace icp::config::file
