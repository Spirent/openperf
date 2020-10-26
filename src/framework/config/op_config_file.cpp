#include <cstdlib>
#include <cstring>
#include <iostream>
#include <numeric>
#include <unordered_map>

#include <unistd.h>

#include "core/op_common.h"
#include "core/op_log.h"
#include "op_config_file.hpp"

namespace openperf::config::file {

using namespace std;

using path_iterator = std::vector<std::string>::const_iterator;
using string_pair = std::pair<std::string, std::string>;

static string config_file_name;
static unordered_map<int, std::string> cli_options;
constexpr static std::string_view path_delimiter(".");
constexpr static std::string_view list_delimiters(" ,");
constexpr static std::string_view pair_delimiter("=");

std::string_view op_config_get_file_name() { return (config_file_name); }

static size_t find_last_of_before(std::string_view input,
                                  std::string_view item_delimiters,
                                  std::string_view kv_delimiters)
{
    auto cursor = input.find_first_of(kv_delimiters);
    if (cursor != std::string::npos) {
        cursor = input.find_last_of(item_delimiters, cursor + 1);
    }

    return (cursor);
}

static std::vector<std::string> split_string(std::string_view input,
                                             std::string_view delimiters)
{
    std::vector<std::string> output;
    size_t beg = 0, pos = 0;
    while ((beg = input.find_first_not_of(delimiters, pos))
           != std::string::npos) {
        pos = input.find_first_of(delimiters, beg + 1);

        output.emplace_back(input.substr(beg, pos - beg));
    }
    return (output);
}

// XXX - replace with the standard library version when we upgrade to C++20.
static bool starts_with(std::string_view val, std::string_view prefix)
{
    return ((val.size() >= prefix.size())
            && (val.compare(0, prefix.size(), prefix) == 0));
}

static std::string trim_left(std::string&& s, std::string_view chars)
{
    s.erase(std::begin(s),
            std::find_if(std::begin(s), std::end(s), [&](auto c) {
                return (chars.find(c) == std::string::npos);
            }));
    return (std::move(s));
}

static std::string trim_right(std::string&& s, std::string_view chars)
{
    s.erase(std::find_if(
                std::rbegin(s),
                std::rend(s),
                [&](auto c) { return (chars.find(c) == std::string::npos); })
                .base(),
            std::end(s));
    return (std::move(s));
}

static std::vector<string_pair> split_map_string(std::string_view input,
                                                 std::string_view delimiters)
{
    std::vector<string_pair> output;
    size_t cursor = 0;
    while (cursor < input.length()) {
        auto key_end = input.find_first_of(pair_delimiter, cursor);

        /* XXX: value_end is relative to key_end + 1 */
        auto value_end = find_last_of_before(
            input.substr(key_end + 1), delimiters, pair_delimiter);

        output.emplace_back(
            trim_left(std::string(input.substr(cursor, key_end - cursor)),
                      delimiters),
            trim_right(std::string(input.substr(key_end + 1, value_end)),
                       delimiters));

        cursor = (value_end == std::string::npos ? std::string::npos
                                                 : key_end + value_end + 2);
    }

    return (output);
}

template <typename StringType>
std::string concatenate(std::vector<StringType>& tokens, std::string_view join)
{
    return (std::accumulate(std::begin(tokens),
                            std::end(tokens),
                            std::string{},
                            [&](auto& lhs, const auto& rhs) {
                                return (lhs +=
                                        ((lhs.empty() ? "" : std::string(join))
                                         + std::string(rhs)));
                            }));
}

static std::vector<std::string>
split_options_string(std::string_view input, std::string_view delimiters)
{
    std::vector<std::string_view> tokens;
    size_t cursor = 0, end = 0;
    while ((cursor = input.find_first_not_of(delimiters, end))
           != std::string::npos) {
        end = input.find_first_of(delimiters, cursor + 1);

        tokens.emplace_back(input.substr(cursor, end - cursor));
    }

    /*
     * Now turn the tokens into a vector of strings, making sure
     * we keep group option arguments together as a single string.
     */
    auto output = std::vector<std::string>{};
    auto opt = std::optional<std::string_view>{};
    auto values = std::vector<std::string_view>{};

    for (auto&& t : tokens) {
        if (starts_with(t, "-")) {
            if (opt) {
                output.emplace_back(*opt);
                if (!values.empty()) {
                    output.emplace_back(concatenate(values, ","));
                    values.clear();
                }
            }

            opt = t;
        } else {
            values.emplace_back(t);
        }
    }

    /* Don't forget the last option! */
    if (opt) {
        output.emplace_back(*opt);
        if (!values.empty()) { output.emplace_back(concatenate(values, ",")); }
    }

    return (output);
}

static void set_map_data_node(YAML::Node& node, std::string_view opt_data)
{
    auto pairs = split_map_string(opt_data, list_delimiters);

    // XXX: yaml-cpp does not have type conversion for unordered_map.
    auto output = std::map<std::string, std::string>{};
    std::transform(std::begin(pairs),
                   std::end(pairs),
                   std::inserter(output, std::end(output)),
                   [](auto&& item) -> string_pair {
                       return {item.first, item.second};
                   });

    node = output;
}

static void set_data_node_value(YAML::Node& node,
                                std::string_view opt_data,
                                enum op_option_type opt_type)
{
    switch (opt_type) {
    case OP_OPTION_TYPE_NONE:
        node = true;
        break;
    case OP_OPTION_TYPE_STRING:
        node = std::string(opt_data);
        break;
    case OP_OPTION_TYPE_HEX:
        node = strtol(opt_data.data(), nullptr, 16);
        break;
    case OP_OPTION_TYPE_LONG:
        node = strtol(opt_data.data(), nullptr, 10);
        break;
    case OP_OPTION_TYPE_DOUBLE:
        node = strtod(opt_data.data(), nullptr);
        break;
    case OP_OPTION_TYPE_MAP:
        set_map_data_node(node, opt_data);
        break;
    case OP_OPTION_TYPE_LIST:
        node = split_string(opt_data, list_delimiters);
        break;
    case OP_OPTION_TYPE_OPTIONS:
        node = split_options_string(opt_data, list_delimiters);
        break;
    }
}

static YAML::Node make_data_node(std::string_view opt_data,
                                 enum op_option_type opt_type)
{
    YAML::Node node;
    set_data_node_value(node, opt_data, opt_type);
    return (node);
}

/*
 * Recursive function to create a new YAML tree path by the given
 * path component strings of the range [pos, end).
 */
static YAML::Node create_param_by_path(path_iterator pos,
                                       const path_iterator end,
                                       std::string_view opt_data,
                                       enum op_option_type opt_type)
{
    if (pos == end) { return (make_data_node(opt_data, opt_type)); }

    YAML::Node output;
    auto key = pos;
    output[*key] = create_param_by_path(++pos, end, opt_data, opt_type);

    return (output);
}

/*
 * Recursive function to traverse an existing YAML tree path by the
 * given path component strings of the range [pos, end). If the entire
 * path exists the base case will assign the requested data value. Else,
 * function will switch over to creating a new path.
 */
static void update_param_by_path(YAML::Node& parent_node,
                                 path_iterator pos,
                                 const path_iterator end,
                                 std::string_view opt_data,
                                 enum op_option_type opt_type)
{
    if (pos == end) {
        set_data_node_value(parent_node, opt_data, opt_type);
        return;
    }

    if (parent_node[*pos]) {
        YAML::Node child_node = parent_node[*pos];
        update_param_by_path(child_node, ++pos, end, opt_data, opt_type);
    } else {
        // Make a copy, else the ++pos operation on the right side will
        // be reflected on the left side.
        auto key = pos;
        parent_node[*key] =
            create_param_by_path(++pos, end, opt_data, opt_type);
    }

    return;
}

static std::optional<YAML::Node> get_param_by_path(
    const YAML::Node& parent_node, path_iterator pos, const path_iterator end)
{
    if (pos == end) { return (parent_node); }

    if (parent_node[*pos]) {
        const YAML::Node child_node = parent_node[*pos];
        return (get_param_by_path(child_node, ++pos, end));
    }

    return (std::nullopt);
}

static void merge_cli_params(std::string_view path, YAML::Node& node)
{
    for (auto& opt : cli_options) {
        auto long_opt = op_options_get_long_opt(opt.first);

        // All CLI options are required to have a long-form version.
        if (!long_opt) {
            throw(std::runtime_error("Command-line option "
                                     + std::to_string(opt.first)
                                     + " does not have a "
                                     + "corresponding long-form " + "option."));
        }

        if (!starts_with(long_opt, path)) { continue; }

        auto arg_path = split_string(long_opt, path_delimiter);

        // Argument names must include the section name plus
        // the structured path for the framework to build out a
        // YAML representation with.
        assert(arg_path.size() > 1);

        update_param_by_path(node,
                             arg_path.begin(),
                             arg_path.end(),
                             opt.second,
                             op_options_get_opt_type_short(opt.first));
    }

    return;
}

std::optional<YAML::Node> op_config_get_param(std::string_view path)
{
    YAML::Node root_node;
    if (!config_file_name.empty()) {
        // If this throws it's a bug or weird environment issue.
        // File is loaded and checked by the CLI option handler below.
        root_node = YAML::LoadFile(config_file_name);
    }

    // Does the user want to override any config file settings
    // from the command line?
    merge_cli_params(path, root_node);

    auto path_components = split_string(path, path_delimiter);

    return (get_param_by_path(
        root_node, path_components.begin(), path_components.end()));
}

static char* find_config_file_option(int argc, char* const argv[])
{
    for (int idx = 0; idx < argc - 1; idx++) {
        if (strcmp(argv[idx], "--config") == 0
            || strcmp(argv[idx], "-c") == 0) {
            return (argv[idx + 1]);
        }
    }

    return (nullptr);
}

extern "C" {
int op_config_file_find(int argc, char* const argv[])
{
    char* file_name = find_config_file_option(argc, argv);

    if (!file_name) { return (0); }

    config_file_name = file_name;

    // Make sure the file exists and is readable.
    if (access(file_name, R_OK) == -1) {
        std::cerr << "Error (" << strerror(errno)
                  << ") while attempting to access config file: " << file_name
                  << std::endl;
        return (errno);
    }

    // This will do an initial parse. yaml-cpp throws exceptions when
    // the parser runs into invalid YAML.
    YAML::Node root_node;
    try {
        root_node = YAML::LoadFile(config_file_name);
    } catch (std::exception& e) {
        std::cerr << "Error parsing configuration file: " << e.what()
                  << std::endl;
        return (EINVAL);
    }

    // XXX: Putting this at the top can lead to the message being lost
    // on its way to the logging thread. Definitely a workaround, but
    // not a critical message either.
    OP_LOG(OP_LOG_DEBUG, "Reading from configuration file %s", file_name);

    // We currently support three top level nodes: `core`, `modules`,
    // and `resources`. Nodes are generally optional, however the
    // `resources` node depends on the `modules` node.  Hence, we return
    // an error if 'resources' exists without `modules`.
    if (root_node["resources"] && !root_node["modules"]) {
        std::cerr << "Configuration file " << file_name
                  << " contains \"resources\""
                  << " but not \"modules\".  The \"modules\" section "
                     "is required."
                  << std::endl;
        return (EINVAL);
    }

    // We also generate a warning if the config file contains
    // unrecognized nodes.
    auto top_level_nodes =
        std::initializer_list<std::string_view>{"core", "modules", "resources"};

    // Clearly, set_difference would be a better choice here, but
    // unfortunately, YAML::Node only appears to allow you to retrieve
    // the key value from an iterator and not from the actual node!
    std::vector<std::string> unknown_nodes;
    for (const auto& node : root_node) {
        auto key = node.first.as<std::string>();
        if (auto found = std::find(
                std::begin(top_level_nodes), std::end(top_level_nodes), key);
            found == std::end(top_level_nodes)) {
            unknown_nodes.push_back(std::move(key));
        }
    }

    if (!unknown_nodes.empty()) {
        OP_LOG(
            OP_LOG_WARNING,
            "Ignoring %zu unrecognized top-level node%s in %s: %s\n",
            unknown_nodes.size(),
            unknown_nodes.size() == 1 ? "" : "s",
            file_name,
            std::accumulate(
                std::begin(unknown_nodes),
                std::end(unknown_nodes),
                std::string(),
                [&](const std::string& a, const std::string& b) -> std::string {
                    return (a
                            + (a.length() == 0
                                   ? ""
                                   : unknown_nodes.size() == 2
                                         ? " and "
                                         : unknown_nodes.back() == b ? ", and "
                                                                     : ". ")
                            + "\\\"" + b + "\\\"");
                })
                .c_str());
    }

    return (0);
}

int framework_cli_option_handler(int opt, const char* opt_arg)
{
    // Check that the user supplied argument data if the registrant
    // developer told us to expect some.
    auto opt_type = (op_options_get_opt_type_short(opt));
    if ((!opt_arg) && (opt_type != OP_OPTION_TYPE_NONE)) { return (EINVAL); }

    // Flag-type arguments are allowed to not have associated data.
    // For those cases the framework creates/updates a node and sets
    // the value to true.
    cli_options[opt] = std::string(opt_arg ? opt_arg : "");

    return (0);
}

char* op_config_file_get_value_str(const char* param, char* value, int len)
{
    assert(param);
    assert(value);
    assert(len);

    auto val = op_config_get_param<std::string>(param);

    if (!val) { return (nullptr); }

    strncpy(value, (*val).c_str(), len);

    return (value);
}

} // extern "C"

} // namespace openperf::config::file
