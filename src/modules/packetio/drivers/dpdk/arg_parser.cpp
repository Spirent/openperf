#include <unordered_map>
#include <regex>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/arg_parser.h"
#include "socket/server/api_server_options.h"

#include <iostream>

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

static const std::string_view port_index_id_regex = "(port)([0-9]+)=([a-z0-9-]+)";
static const size_t port_index_match = 2;
static const size_t port_id_match = 3;
// Input "port0=port_zero" is parsed into the following submatches:
// match 0: "port0=port_zero"
// match 1: "port"
// match 2: "0"
// match 3: "port_zero"

#define PRINT_NAME_ERROR(id_)                          \
    std::cerr << std::string(id_)                      \
    + " is not a valid port id specifier."             \
    + " It must have the form portX=id,"               \
    + " where X is the zero-based DPDK port index and" \
    + " where id may only contain"                     \
    + " lower-case letters, numbers, and hyphens."     \
    << std::endl

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
        return (EINVAL);
    }
    if (icp_options_hash_long("dpdk-port-ids") == opt) {
        if (opt_arg != nullptr) {
            // Pick out all the portX=name pairs.
            std::string_view input(opt_arg);
            std::string_view delimiters(" ,");
            size_t beg = 0, pos = 0;
            auto data_pair_regex = std::regex(port_index_id_regex.data(), std::regex::extended);
            while ((beg = input.find_first_not_of(delimiters, pos)) != std::string::npos) {
                pos = input.find_first_of(delimiters, beg + 1);

                std::string_view index_id_pair(input.substr(beg, pos - beg));

                std::cmatch index_id_pieces;
                if (std::regex_match(index_id_pair.begin(), index_id_pair.end(),
                                     index_id_pieces, data_pair_regex)) {
                    // regex_match returns the whole match as item 0, and the
                    // sub matches as the remaining items.
                    if (index_id_pieces.size() != 4) {
                        PRINT_NAME_ERROR(index_id_pair);
                    }

                    int port_index = std::stoi(index_id_pieces.str(port_index_match));
                    std::string port_id = index_id_pieces.str(port_id_match);

                    if (m_port_index_id.find(port_index) != m_port_index_id.end()) {
                        std::cout << "Error: detected a duplicate port index: "
                                  << std::to_string(port_index) << std::endl;
                        return (EINVAL);
                    }
                    auto it =
                        std::find_if(m_port_index_id.begin(), m_port_index_id.end(),
                                     [&port_id](const std::pair<int, std::string> &val) {
                                         return val.second == port_id;
                                     });
                    if (it != m_port_index_id.end()) {
                        std::cout << "Error: detected a duplicate port id: "
                                  << port_id << std::endl;
                        return (EINVAL);
                    }

                    m_port_index_id[port_index] = port_id;
                } else {
                    PRINT_NAME_ERROR(index_id_pair);
                    return (EINVAL);
                }
            }

            return (0);
        }

        return(EINVAL);
    }

    if (opt != 'd' || opt_arg == nullptr) {
        return (EINVAL);
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

#undef PRINT_NAME_ERROR

int arg_parser::test_portpairs()
{
    return (m_test_portpairs);
}

bool arg_parser::test_mode()
{
    return (m_test_mode);
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

std::map<int, std::string> arg_parser::id_map()
{
    return (m_port_index_id);
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
