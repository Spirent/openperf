
#include "config/icp_config_file.h"
#include "config/icp_config_file_utils.h"
#include "api_utils.h"
#include "yaml-cpp/yaml.h"
#include "api_internal_client.h"
#include "api/api_config_file_resources.h"

using namespace icp::config::file;

namespace icp::api::config {

tl::expected<void, std::string> icp_config_file_process_resources()
{
    const char *config_file_name = icp_get_config_file_name();
    if (strlen(config_file_name) == 0) return {};

    // Make sure the API module is up and running.
    // This function will terminate Inception if it's not running.
    auto res = utils::check_api_module_running();
    if (!res) return (res);

    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception &e) {
        return (tl::make_unexpected("Error parsing configuration file: " + std::string(e.what())));
    }

    for (auto resource : root_node["resources"]) {
        auto [path, id] = icp_config_split_path_id(resource.first.Scalar());

        std::string jsonified_resource;
        try {
            icp_config_yaml_to_json(resource.second, jsonified_resource);
        } catch (...) {
            return (tl::make_unexpected(
              "Error processing resource: " + resource.first.Scalar()
              + ". Issue converting input YAML format to REST-compatible JSON format."));
        }

        auto [code, body] = icp::api::client::internal_api_post(path, jsonified_resource);

        switch (code) {
        case Pistache::Http::Code::Ok:
            break;
        case Pistache::Http::Code::Not_Found:
            return (tl::make_unexpected("Error configuring resource: " + std::string(path.data())
                                        + ". Invalid path: " + std::string(path)));
            break;
        case Pistache::Http::Code::Method_Not_Allowed:
            // Config trying to POST to a resource that does not support POST.
            return (tl::make_unexpected("Error configuring resource: " + std::string(path.data())
                                        + ". " + std::string(path)
                                        + " is read-only and not configurable."));
            break;
        default:
            return (tl::make_unexpected("Error while processing resource: "
                                        + resource.first.as<std::string>()
                                        + " Reason: " + body.c_str()));
            break;
        }
    }

    return {};
}
}  // namespace icp::api::config
