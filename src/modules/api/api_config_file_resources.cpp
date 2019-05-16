
#include "config/icp_config_file.h"
#include "config/icp_config_file_utils.h"
#include "api_utils.h"
#include "yaml-cpp/yaml.h"
#include "api_internal_client.h"

using namespace icp::config::file;

namespace icp::api::config {

int icp_config_file_process_resources()
{
    const char *config_file_name = icp_get_config_file_name();
    if (strlen(config_file_name) == 0) return (0);

    // Make sure the API module is up and running.
    // This function will terminate Inception if it's not running.
    if (utils::check_api_module_running() == -1) return (-1);

    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception &e) {
        std::cerr << "Error parsing configuration file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    for (auto resource : root_node["resources"]) {
        auto [path, id] = icp_config_split_path_id(resource.first.Scalar());

        std::string jsonified_resource;
        try {
            icp_config_yaml_to_json(resource.second, jsonified_resource);
        } catch (...) {
            std::cerr << "Error processing resource: " << resource.first.Scalar()
                      << ". Issue converting input YAML format to REST-compatible JSON format."
                      << std::endl;
            exit(EXIT_FAILURE);
        }

        auto [code, body] = icp::api::client::internal_api_post(path, jsonified_resource);

        switch (code) {
        case Pistache::Http::Code::Ok:
            break;
        case Pistache::Http::Code::Not_Found:
            std::cerr << "Error configuring resource: " << path.data()
                      << ". Invalid path: " << std::string(path) << std::endl;
            exit(EXIT_FAILURE);
            break;
        case Pistache::Http::Code::Method_Not_Allowed:
            // Config trying to POST to a resource that does not support POST.
            std::cerr << "Error configuring resource: " << path.data() << ". " << std::string(path)
                      << " is read-only and not configurable." << std::endl;
            exit(EXIT_FAILURE);
            break;
        default:
            std::cerr << "Error while processing resource: "
                      << resource.first.as<std::string>().c_str() << " Reason: " << body.c_str()
                      << std::endl;
            exit(EXIT_FAILURE);
            break;
        }
    }

    return (0);
}
}  // namespace icp::api::config
