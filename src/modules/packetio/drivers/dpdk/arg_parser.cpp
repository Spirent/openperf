#include <unordered_map>
#include <regex>

#include "core/op_core.h"
#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/arg_parser.hpp"
#include "socket/server/api_server_options.h"

#include <iostream>

namespace openperf::packetio::dpdk::config {

using namespace openperf::config;

static constexpr std::string_view program_name = "op_eal";

/* Check if the 'log-level' argument has been added to the arguments vector */
static bool have_log_level_arg(std::vector<std::string>& args)
{
    for (const auto& s : args) {
        if (s == "--log-level") { return (true); }
    }
    return (false);
}

static void add_log_level_arg(enum op_log_level level,
                              std::vector<std::string>& args)
{
    /* Map OP log levels to DPDK log levels */
    static std::unordered_map<enum op_log_level, std::string> log_level_map = {
        {OP_LOG_NONE, "0"},     /* RTE_LOG_EMERG */
        {OP_LOG_CRITICAL, "1"}, /* RTE_LOG_ERERG */
        {OP_LOG_ERROR, "2"},    /* RTE_LOG_ALERT */
        {OP_LOG_WARNING, "3"},  /* RTE_LOG_CRIT */
        {OP_LOG_INFO, "4"},     /* RTE_LOG_ERR */
        {OP_LOG_DEBUG, "7"},    /* RTE_LOG_INFO */
        {OP_LOG_TRACE, "8"}     /* RTE_LOG_DEBUG */
    };

    args.push_back("--log-level");
    args.push_back(log_level_map[level]);
}

static bool have_file_prefix_arg(std::vector<std::string>& args)
{
    for (const auto& s : args) {
        if (s == "--file-prefix") { return (true); }
    }
    return (false);
}

static void add_file_prefix_arg(const char* prefix,
                                std::vector<std::string>& args)
{
    args.push_back("--file-prefix");
    args.push_back(prefix);
}

static bool have_no_pci_arg(std::vector<std::string>& args)
{
    for (const auto& s : args) {
        if (s == "--no-pci") { return (true); }
    }
    return (false);
}

static void add_no_pci_arg(std::vector<std::string>& args)
{
    args.push_back("--no-pci");
}

// Split portX and return just the X part.
// Return -1 if no valid X part is found.
static int get_port_index(std::string_view name)
{
    auto index_offset = name.find_first_not_of("port");
    if (index_offset == std::string_view::npos) { return (-1); }

    char* last_char;
    auto to_return = strtol(name.data() + index_offset, &last_char, 10);
    if (*last_char == '\0') { return (to_return); }

    return (-1);
}

static int
process_dpdk_port_ids(const std::map<std::string, std::string>& input,
                      std::unordered_map<int, std::string>& output)
{
    for (auto& entry : input) {
        // split port index from "port" part.
        auto port_idx = get_port_index(entry.first);
        if (port_idx < 0) {
            std::cerr << std::string(entry.first)
                             + " is not a valid port id specifier."
                             + " It must have the form portX=id,"
                             + " where X is the zero-based DPDK port index and"
                             + " where id may only contain"
                             + " lower-case letters, numbers, and hyphens."
                      << std::endl;
            return (EINVAL);
        }

        // check for duplicate port index.
        if (output.find(port_idx) != output.end()) {
            std::cerr << "Error: detected a duplicate port index: " << port_idx
                      << std::endl;
            return (EINVAL);
        }

        // check for duplicate port ID.
        auto port_id = entry.second;
        auto it =
            std::find_if(output.begin(), output.end(),
                         [&port_id](const std::pair<int, std::string>& val) {
                             return val.second == port_id;
                         });
        if (it != output.end()) {
            std::cerr << "Error: detected a duplicate port id: " << port_id
                      << std::endl;
            return (EINVAL);
        }

        output[port_idx] = port_id;
    }

    return (0);
}

int dpdk_test_portpairs()
{
    auto result = config::file::op_config_get_param<OP_OPTION_TYPE_LONG>(
        "modules.packetio.dpdk.test-portpairs");

    return (result.value_or(dpdk_test_mode() ? 1 : 0));
}

bool dpdk_test_mode()
{
    auto result = config::file::op_config_get_param<OP_OPTION_TYPE_NONE>(
        "modules.packetio.dpdk.test-mode");

    return (result.value_or(false));
}

/*
 * Our list argument type splits arguments on commas, however some DPDK options
 * use commas to support key=value modifiers.  This function reconstructs the
 * commas so that arguments are parsed correctly.
 */
static void add_dpdk_argument(std::vector<std::string>& args,
                              std::string_view input)
{
    if (args.empty() || input.front() == '-'
        || (!args.empty() && args.back().front() == '-')) {
        args.emplace_back(input);
    } else {
        args.back().append("." + std::string(input));
    }
}

std::vector<std::string> dpdk_args()
{
    // Add name value in straight away.
    std::vector<std::string> to_return{std::string(program_name)};

    // Get the list from the framework.
    auto arg_list = config::file::op_config_get_param<OP_OPTION_TYPE_LIST>(
        "modules.packetio.dpdk.options");
    if (!arg_list) { return (to_return); }

    // Walk through it and rebuild the arguments DPDK expects
    for (auto& v : *arg_list) { add_dpdk_argument(to_return, v); }

    /* Append a log level option if needed */
    if (!have_log_level_arg(to_return)) {
        add_log_level_arg(op_log_level_get(), to_return);
    }
    if (!have_file_prefix_arg(to_return)) {
        if (auto prefix = api_server_options_prefix_option_get();
            prefix != nullptr && prefix[0] != '\0') {
            add_file_prefix_arg(prefix, to_return);
        }
    }
    if (dpdk_test_mode() && !have_no_pci_arg(to_return)) {
        add_no_pci_arg(to_return);
    }

    return (to_return);
}

std::unordered_map<int, std::string> dpdk_id_map()
{
    auto src_map = config::file::op_config_get_param<OP_OPTION_TYPE_MAP>(
        "modules.packetio.dpdk.port-ids");

    std::unordered_map<int, std::string> to_return;

    if (!src_map) { return (to_return); }

    if (process_dpdk_port_ids(*src_map, to_return) != 0) {
        throw std::runtime_error(
            "Error mapping DPDK Port Indexes to Port IDs.");
    }

    return (to_return);
}

} /* namespace openperf::packetio::dpdk::config */
