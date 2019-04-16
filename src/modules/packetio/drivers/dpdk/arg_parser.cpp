#include <unordered_map>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/arg_parser.h"
#include "socket/server/api_server_options.h"

namespace icp {
namespace packetio {
namespace dpdk {

/* Check if the 'log-level' argument has been added to the arguments vector */
static bool have_log_level_arg(std::vector<std::string> &args)
{
    for (const auto &s : args) {
        if (s == "--log-level") {
            return (true);
        }
    }
    return (false);
}

static void add_log_level_arg(enum icp_log_level level, std::vector<std::string>& args)
{
    /* Map ICP log levels to DPDK log levels */
    static std::unordered_map<enum icp_log_level, std::string> log_level_map = {
        {ICP_LOG_NONE,     "0"},  /* RTE_LOG_EMERG */
        {ICP_LOG_CRITICAL, "1"},  /* RTE_LOG_ERERG */
        {ICP_LOG_ERROR,    "2"},  /* RTE_LOG_ALERT */
        {ICP_LOG_WARNING,  "3"},  /* RTE_LOG_CRIT */
        {ICP_LOG_INFO,     "4"},  /* RTE_LOG_ERR */
        {ICP_LOG_DEBUG,    "7"},  /* RTE_LOG_INFO */
        {ICP_LOG_TRACE,    "8"}   /* RTE_LOG_DEBUG */
    };

    args.push_back("--log-level");
    args.push_back(log_level_map[level]);
}

static bool have_file_prefix_arg(std::vector<std::string> &args)
{
    for (const auto &s : args) {
        if (s == "--file-prefix") {
            return (true);
        }
    }
    return (false);
}

static void add_file_prefix_arg(const char* prefix, std::vector<std::string>& args)
{
    args.push_back("--file-prefix");
    args.push_back(prefix);
}

static bool have_no_pci_arg(std::vector<std::string> &args)
{
    for (const auto &s : args) {
        if (s == "--no-pci") {
            return (true);
        }
    }
    return (false);
}

static void add_no_pci_arg(std::vector<std::string>& args)
{
    args.push_back("--no-pci");
}

int arg_parser::init(const char *name)
{
    _args.clear();
    _args.push_back(std::string(name));
    m_test_mode = false;
    m_test_portpairs = 0;
    return (0);
}

int arg_parser::parse(int opt, const char *opt_arg)
{
    if (icp_options_hash_long("dpdk-test-mode") == opt) {
        m_test_mode = true;
        m_test_portpairs = 1;
        return (0);
    }
    if (icp_options_hash_long("dpdk-test-portpairs") == opt) {
        if (opt_arg != nullptr) {
            m_test_portpairs = std::strtol(opt_arg, nullptr, 10);
            return (0);
        }
        return (-EINVAL);
    }
    if (opt != 'd' || opt_arg == nullptr) {
        return (-EINVAL);
    }

    /* Assume optarg is a comma seperated list of DPDK/EAL options */
    std::string input(opt_arg);
    std::string delimiters(" ,=");
    size_t beg = 0, pos = 0;
    while ((beg = input.find_first_not_of(delimiters, pos)) != std::string::npos) {
        pos = input.find_first_of(delimiters, beg + 1);
        _args.emplace_back(input.substr(beg, pos-beg));
    }

    return (0);
}

int arg_parser::test_portpairs()
{
    return (m_test_portpairs);
}

std::vector<std::string> arg_parser::args()
{
    std::vector<std::string> to_return;
    std::copy(begin(_args), end(_args), std::back_inserter(to_return));

    /* Append a log level option if needed */
    if (!have_log_level_arg(to_return)) {
        add_log_level_arg(icp_log_level_get(), to_return);
    }
    if (!have_file_prefix_arg(to_return)) {
        if (auto prefix = api_server_options_prefix_option_get(); prefix != nullptr
            && prefix[0] != '\0') {
            add_file_prefix_arg(prefix, to_return);
        }
    }
    if (m_test_mode && !have_no_pci_arg(to_return)) {
        add_no_pci_arg(to_return);
    }

    return (to_return);
}

}
}
}

/**
 * Provide C hooks for icp_options to use.
 */
extern "C" {

using namespace icp::packetio::dpdk;

int dpdk_arg_parse_init()
{
    /* Arguments need to start with the program name */
    auto& parser = arg_parser::instance();
    return (parser.init("icp_eal"));
}

int dpdk_arg_parse_handler(int opt, const char *opt_arg)
{
    auto& parser = arg_parser::instance();
    return (parser.parse(opt, opt_arg));
}

}
