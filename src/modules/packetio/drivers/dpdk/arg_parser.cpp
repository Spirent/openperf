#include <unordered_map>
#include <regex>

#include "core/op_core.h"
#include "config/op_config_file.hpp"
#include "config/op_config_prefix.hpp"
#include "packetio/drivers/dpdk/arg_parser.hpp"

#include <iostream>

namespace openperf::packetio::dpdk::config {

using namespace openperf::config;

inline constexpr std::string_view program_name = "op_eal";
inline constexpr std::string_view log_level_arg = "--log-level";
inline constexpr std::string_view file_prefix_arg = "--file-prefix";

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

    args.emplace_back(log_level_arg);
    args.push_back(log_level_map[level]);
}

static void add_file_prefix_arg(std::string_view prefix,
                                std::vector<std::string>& args)
{
    args.emplace_back(file_prefix_arg);
    args.emplace_back(prefix);
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
                      std::map<uint16_t, std::string>& output)
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
            std::find_if(output.begin(),
                         output.end(),
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

std::vector<std::string> common_dpdk_args()
{
    // Add name value in straight away.
    std::vector<std::string> to_return{std::string(program_name)};

    // Get the list from the framework.
    auto arg_list = config::file::op_config_get_param<OP_OPTION_TYPE_LIST>(
        op_packetio_dpdk_options);
    if (arg_list) {
        // Walk through it and rebuild the arguments DPDK expects
        for (auto& v : *arg_list) { add_dpdk_argument(to_return, v); }
    }

    /* Append a log level option if needed */
    if (!contains(to_return, log_level_arg)) {
        add_log_level_arg(op_log_level_get(), to_return);
    }
    if (!contains(to_return, file_prefix_arg)) {
        if (auto prefix = config::get_prefix()) {
            add_file_prefix_arg(*prefix, to_return);
        }
    }

    return (to_return);
}

std::map<uint16_t, std::string> dpdk_id_map()
{
    auto src_map = config::file::op_config_get_param<OP_OPTION_TYPE_MAP>(
        op_packetio_dpdk_port_ids);

    std::map<uint16_t, std::string> to_return;

    if (!src_map) { return (to_return); }

    if (process_dpdk_port_ids(*src_map, to_return) != 0) {
        throw std::runtime_error(
            "Error mapping DPDK Port Indexes to Port IDs.");
    }

    return (to_return);
}

std::optional<core_mask> misc_core_mask()
{
    const auto mask = config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
        op_packetio_dpdk_misc_worker_mask);

    if (mask) { return core_mask{*mask}; }

    return (std::nullopt);
}

std::optional<core_mask> rx_core_mask()
{
    const auto mask = config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
        op_packetio_dpdk_rx_worker_mask);

    if (mask) { return core_mask{*mask}; }

    return (std::nullopt);
}

std::optional<core_mask> tx_core_mask()
{
    const auto mask = config::file::op_config_get_param<OP_OPTION_TYPE_HEX>(
        op_packetio_dpdk_tx_worker_mask);

    if (mask) { return core_mask{*mask}; }

    return (std::nullopt);
}

} /* namespace openperf::packetio::dpdk::config */
