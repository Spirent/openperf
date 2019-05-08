#include "core/icp_common.h"
#include "icp_config_file.h"
#include "config_file_utils.h"
#include "api/api_internal_client.h"
#include "api/api_service.h"
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <math.h>

namespace icp::config::file {

using namespace std;

static string config_file_name;
static const string api_server_host          = "localhost";
static const string api_check_resource       = "/version";
static constexpr unsigned int poll_interval  = 10;  // in ms
static constexpr unsigned int max_poll_count = 6;

// Is the API port open?
static void check_api_port()
{
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    int res = getaddrinfo(api_server_host.c_str(), to_string(api::api_get_service_port()).c_str(),
                          &hints, &result);
    if (res != 0) {
        std::cerr << "Error starting up internal API client: " << gai_strerror(res) << std::endl;
        exit(EXIT_FAILURE);
    }

    unsigned int poll_count = 0;
    bool done               = false;
    for (; (poll_count < max_poll_count) && !done; poll_count++) {
        // Experiments showed that a delay is always required.
        // Sleep first, check later.
        std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval * (1 << poll_count)));

        int sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
        if (sockfd == -1) {
            std::cerr << "Error starting up internal API client: " << strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, result->ai_addr, result->ai_addrlen) == 0) { done = true; }

        close(sockfd);
    }

    freeaddrinfo(result);

    if (!done) {
        std::cerr << "Error starting up internal API client. Could not connect to API server."
                  << std::endl;
        exit(EXIT_FAILURE);
    }
}

// Is the API able to retrieve a resource?
static void check_api_resource()
{
    unsigned int poll_count = 0;
    bool done               = false;

    for (; (poll_count < max_poll_count) && !done; poll_count++) {
        auto [code, body] = icp::api::client::internal_api_get(api_check_resource);
        if (code == Pistache::Http::Code::Ok) {
            done = true;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval * (1 << poll_count)));
    }

    if (!done) {
        std::cerr << "Error starting up internal API client. Could not retrieve resource: "
                  << api_check_resource.c_str() << std::endl;
        exit(EXIT_FAILURE);
    }
}

static void check_api_module_running()
{
    check_api_port();

    check_api_resource();
}

extern "C" {
int config_file_option_handler(int opt, const char *opt_arg)
{
    if (opt != 'c' || opt_arg == nullptr) { return (-EINVAL); }

    config_file_name = opt_arg;

    // Make sure the file exists and is readable.
    if (access(opt_arg, R_OK) == -1) {
        std::cerr << "Error (" << strerror(errno)
                  << ") while attempting to access config file: " << opt_arg << std::endl;
        exit(EXIT_FAILURE);
    }

    // This will do an initial parse. yaml-cpp throws exceptions when the parser runs into invalid
    // YAML.
    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception &e) {
        std::cerr << "Error parsing configuration file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // Validate there are the two required top-level nodes "core" and "resources".
    // Also check for the optional "modules" resource, and that no other nodes exist.
    if ((root_node.size() == 2) && root_node["core"] && root_node["resources"]) {
        return (0);
    } else if ((root_node.size() == 3) && root_node["core"] && root_node["resources"]
               && root_node["modules"]) {
        return (0);
    }

    std::cerr << "Configuration file must only contain top-level sections \"core:\" and "
                 "\"resources\", and, optionally, \"modules:\""
              << std::endl;
    exit(EXIT_FAILURE);

    return (-1);
}

int icp_config_file_process_core(int(process_val)(int, const char *))
{
    YAML::Node root;
    try {
        root = YAML::LoadFile(config_file_name);
    } catch (YAML::BadFile) {
        cout << "file not found!!" << endl;
        return -1;
    }

    // process_val(0, nullptr);
    return (0);
}

int icp_config_file_process_resources()
{
    if (config_file_name.empty()) return (0);

    // Make sure the API module is up and running.
    // This function will terminate Inception if it's not running.
    check_api_module_running();

    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception &e) {
        std::cerr << "Error parsing configuration file: " << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }

    // FIXME: Is resource count == 0 an error?

    for (auto resource : root_node["resources"]) {
        auto [path, id] = icp_config_split_path_id(resource.first.Scalar());

        std::string output_string;
        try {
            icp_config_yaml_to_json(resource.second, output_string);
        } catch (...) {
            std::cerr << "Error processing resource: " << resource.first.Scalar()
                      << ". Issue converting input YAML format to REST-compatible JSON format."
                      << std::endl;
            exit(EXIT_FAILURE);
        }

        auto [code, body] = icp::api::client::internal_api_post(path, output_string);

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

}  // extern "C"

}  // namespace icp::config::file
