#include "core/icp_common.h"
#include "icp_config_file.h"
#include <iostream>
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <cstdlib>
#include "core/icp_log.h"

namespace icp::config::file {

using namespace std;

using path_iterator = std::vector<std::string>::const_iterator;

static string config_file_name;
static unordered_map<int, std::string> cli_options;
constexpr static std::string_view path_delimiter(".");
constexpr static std::string_view list_delimiters(" ,");
constexpr static std::string_view pair_delimiter("=");

std::string_view icp_config_get_file_name()
{
    return (config_file_name);
}

static std::vector<std::string> split_string(std::string_view input, std::string_view delimiters)
{
    std::vector<std::string> output;
    size_t beg = 0, pos = 0;
    while ((beg = input.find_first_not_of(delimiters, pos)) != std::string::npos) {
        pos = input.find_first_of(delimiters, beg + 1);

        output.push_back(std::string(input.substr(beg, pos - beg)));
    }
    return (output);
}

// XXX - replace with the standard library version when we upgrade to C++20.
static bool starts_with(std::string_view val, std::string_view prefix)
{
    return ((val.size() >= prefix.size()) && (val.compare(0, prefix.size(), prefix) == 0));
}

static void set_map_data_node(YAML::Node &node, std::string_view opt_data)
{
    auto data_pairs = split_string(opt_data, list_delimiters);

    // XXX: yaml-cpp does not have type conversion for unordered_map.
    std::map<std::string, std::string> output;
    for (auto &data_pair : data_pairs) {
        size_t pos = data_pair.find_first_of(pair_delimiter);
        if (pos == std::string::npos) { continue; }

        output[data_pair.substr(0, pos)] = data_pair.substr(pos + 1, data_pair.length());
    }

    node = output;
}

static void set_data_node_value(YAML::Node &node, std::string_view opt_data,
                                enum icp_option_type opt_type)
{
    switch (opt_type) {
    case ICP_OPTION_TYPE_STRING:
        node = std::string(opt_data);
        break;
    case ICP_OPTION_TYPE_LONG:
        node = strtol(opt_data.data(), nullptr, 10);
        break;
    case ICP_OPTION_TYPE_MAP:
        set_map_data_node(node, opt_data);
        break;
    case ICP_OPTION_TYPE_LIST:
        node = split_string(opt_data, list_delimiters);
        break;
    case ICP_OPTION_TYPE_DOUBLE:
        node = strtod(opt_data.data(), nullptr);
        break;
    case ICP_OPTION_TYPE_NONE:
        node = true;
        break;
    }

    return;
}

static YAML::Node make_data_node(std::string_view opt_data, enum icp_option_type opt_type)
{
    YAML::Node node;
    set_data_node_value(node, opt_data, opt_type);
    return (node);
}

/*
 * Recursive function to create a new YAML tree path by the given
 * path component strings of the range [pos, end).
 */
static YAML::Node create_param_by_path(path_iterator pos, const path_iterator end,
                                       std::string_view opt_data, enum icp_option_type opt_type)
{
    if (pos == end) { return (make_data_node(opt_data, opt_type)); }

    YAML::Node output;
    auto key = pos;
    output[*key] = create_param_by_path(++pos, end, opt_data, opt_type);

    return (output);
}

/*
 * Recursive function to traverse an existing YAML tree path by the given
 * path component strings of the range [pos, end). If the entire path exists
 * the base case will assign the requested data value. Else, function will
 * switch over to creating a new path.
 */
static void update_param_by_path(YAML::Node &parent_node, path_iterator pos,
                                 const path_iterator end, std::string_view opt_data,
                                 enum icp_option_type opt_type)
{
    if (pos == end) {
        set_data_node_value(parent_node, opt_data, opt_type);
        return;
    }

    if (parent_node[*pos]) {
        YAML::Node child_node = parent_node[*pos];
        update_param_by_path(child_node, ++pos, end, opt_data, opt_type);
    } else {
        // Make a copy, else the ++pos operation on the right side will be reflected
        // on the left side.
        auto key = pos;
        parent_node[*key] = create_param_by_path(++pos, end, opt_data, opt_type);
    }

    return;
}

static std::optional<YAML::Node> get_param_by_path(const YAML::Node &parent_node,
                                                   path_iterator pos,
                                                   const path_iterator end)
{
    if (pos == end) { return (parent_node); }

    if (parent_node[*pos]) {
        const YAML::Node child_node = parent_node[*pos];
        return (get_param_by_path(child_node, ++pos, end));
    }

    return (std::nullopt);
}

static void merge_cli_params(std::string_view path, YAML::Node &node)
{
    for (auto &opt : cli_options) {
        auto long_opt = icp_options_get_long_opt(opt.first);

        // All CLI options are required to have a long-form version.
        assert(long_opt);

        if (!long_opt) { throw (std::runtime_error("Command-line option "
                                                     + std::to_string(opt.first)
                                                     + " does not have a "
                                                     + "corresponding long-form "
                                                     + "option.")); }

        if (!starts_with(long_opt, path)) { continue; }

        auto arg_path = split_string(long_opt, path_delimiter);

        // Argument names must include the section name plus
        // the structured path for the framework to build out a
        // YAML representation with.
        assert(arg_path.size() > 1);

        update_param_by_path(node, arg_path.begin(), arg_path.end(), opt.second,
                             icp_options_get_opt_type_short(opt.first));
    }

    return;
}

std::optional<YAML::Node> icp_config_get_param(std::string_view path)
{
    YAML::Node root_node;
    if (!config_file_name.empty()) {
        try {
            root_node = YAML::LoadFile(config_file_name);
        } catch (std::exception &e) {
            throw;
        }
    }

    // Does the user want to override any config file settings
    // from the command line?
    merge_cli_params(path, root_node);

    auto path_components = split_string(path, path_delimiter);

    return (get_param_by_path(root_node, path_components.begin(), path_components.end()));
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

int framework_cli_option_handler(int opt, const char *opt_arg)
{
    // Check that the user supplied argument data if the registrant
    // developer told us to expect some.
    auto opt_type = (icp_options_get_opt_type_short(opt));
    if  ((!opt_arg) && (opt_type != ICP_OPTION_TYPE_NONE)) {
        return (EINVAL);
    }

    // Flag-type arguments are allowed to not have associated data.
    // For those cases the framework creates/updates a node and sets
    // the value to true.
    cli_options[opt] = std::string(opt_arg ? opt_arg : "");

    return (0);
}

}  // extern "C"

}  // namespace icp::config::file
