#include <unordered_map>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/arg_parser.h"

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

int arg_parser::init(const char *name)
{
    _args.clear();
    _args.push_back(std::string(name));
    return (0);
}

int arg_parser::parse(int opt, const char *opt_arg)
{
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

std::vector<std::string> arg_parser::args()
{
    std::vector<std::string> to_return;
    std::copy(begin(_args), end(_args), std::back_inserter(to_return));

    /* Append a log level option if needed */
    if (!have_log_level_arg(to_return)) {
        add_log_level_arg(icp_log_level_get(), to_return);
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

int dpdk_arg_parse_init(void *opt_data __attribute__((unused)))
{
    /* Arguments need to start with the program name */
    auto& parser = arg_parser::instance();
    return (parser.init("icp_eal"));
}

int dpdk_arg_parse_handler(int opt, const char *opt_arg,
                           void *opt_data __attribute__((unused)))
{
    auto& parser = arg_parser::instance();
    return (parser.parse(opt, opt_arg));
}

}
