#include <cerrno>
#include <cstdlib>

#include <net/if.h>
#include <sys/capability.h>

#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

#include "core/icp_core.h"
#include "drivers/dpdk/dpdk.h"
#include "packetio/component.h"

namespace icp {
namespace packetio {
namespace driver {

template <typename T>
class singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator= (const singleton) = delete;

protected:
    singleton() {};
};

struct arg_parser : public singleton<arg_parser>
{
    int init(const char *name)
    {
        args.clear();
        args.push_back(std::string(name));
        return (0);
    }

    int parse(int opt, const char *opt_arg)
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
            args.emplace_back(input.substr(beg, pos-beg));
        }

        return (0);
    }

    /* Check if the 'log-level' argument has been added to the arguments vector */
    bool have_log_level_arg()
    {
        for (const auto &s : args) {
            if (s == "--log-level") {
                return (true);
            }
        }
        return (false);
    }

    void add_log_level_arg(enum icp_log_level level)
    {
        /* Map ICP log levels to DPDK log levels */
        static std::unordered_map<enum icp_log_level, std::string> log_level_map = {
            {ICP_LOG_NONE,     "1"},  /* RTE_LOG_EMERG */
            {ICP_LOG_CRITICAL, "3"},  /* RTE_LOG_CRIT */
            {ICP_LOG_ERROR,    "4"},  /* RTE_LOG_ERR */
            {ICP_LOG_WARNING,  "5"},  /* RTE_LOG_WARNING */
            {ICP_LOG_INFO,     "7"},  /* RTE_LOG_INFO */
            {ICP_LOG_DEBUG,    "8"},  /* RTE_LOG_DEBUG */
            {ICP_LOG_TRACE,    "8"}
        };

        args.push_back("--log-level");
        args.push_back(log_level_map[level]);
    }

    std::vector<std::string> args;
};

struct named_cap_flag {
    const char *name;
    int flag;
};

static struct named_cap_flag cap_permissions[] = {
    { "cap_ipc_lock", CAP_IPC_LOCK },
    { "cap_net_raw",  CAP_NET_RAW  },
};

static bool sufficient_permissions()
{
    cap_t caps = cap_get_proc();
    if (!caps) {
        icp_log(ICP_LOG_ERROR, "Could not retrieve any capabilities.\n");
        return (false);
    }

    bool have_permissions = true;  /* assume we have permission unless we don't */
    for(auto &item : cap_permissions) {
        cap_flag_value_t flag;
        if (cap_get_flag(caps, item.flag, CAP_EFFECTIVE, &flag) == -1) {
            icp_log(ICP_LOG_ERROR, "cap_get_flag returned error for %s.\n", item.name);
            have_permissions = false;
            break;
        } else if (!flag) {
            icp_log(ICP_LOG_INFO, "Missing required DPDK capability: %s\n", item.name);
            have_permissions = false;
        }
    }

    cap_free(caps);

    return (have_permissions);
}

static void log_port_info(uint16_t port_idx)
{
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port_idx, &info);

    struct ether_addr mac_addr;
    rte_eth_macaddr_get(port_idx, &mac_addr);

    if (info.if_index > 0) {
        char if_name[IF_NAMESIZE];
        if_indextoname(info.if_index, if_name);
        icp_log(ICP_LOG_INFO, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s) attached to %s\n",
                port_idx,
                mac_addr.addr_bytes[0],
                mac_addr.addr_bytes[1],
                mac_addr.addr_bytes[2],
                mac_addr.addr_bytes[3],
                mac_addr.addr_bytes[4],
                mac_addr.addr_bytes[5],
                info.driver_name, if_name);
    } else {
        icp_log(ICP_LOG_INFO, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s)\n",
                port_idx,
                mac_addr.addr_bytes[0],
                mac_addr.addr_bytes[1],
                mac_addr.addr_bytes[2],
                mac_addr.addr_bytes[3],
                mac_addr.addr_bytes[4],
                mac_addr.addr_bytes[5],
                info.driver_name);
    }
}

static void init()
{
    /* Check to see if we have permissions to launch DPDK */
    if (!sufficient_permissions()) {
        throw std::runtime_error("Insufficient permissions to initialize DPDK");
    }

    /* DPDK uses getopt to parse command line options, so reset getopt */
    optind = 0;
    opterr = 0;

    /* See if we should add a log-level argument */
    auto& parser = arg_parser::instance();
    if (!parser.have_log_level_arg()) {
        parser.add_log_level_arg(icp_log_level_get());
    }

    /* Convert args to c-strings */
    std::vector<char *> dpdk_args;
    for (const auto& arg : parser.args) {
        dpdk_args.push_back(const_cast<char *>(arg.data()));
    }
    dpdk_args.push_back(nullptr);

    icp_log(ICP_LOG_DEBUG, "Initializing DPDK with \"%s\"\n",
            std::accumulate(begin(parser.args), end(parser.args), std::string(),
                            [](const std::string &a, const std::string &b) -> std::string {
                                return a + (a.length() > 0 ? " " : "") + b;
                            }).c_str());

    int ret = rte_eal_init(dpdk_args.size() - 1, dpdk_args.data());
    if (ret < 0) {
        throw std::runtime_error("Failed to initialize DPDK");
    }

    uint16_t port_idx = 0;
    RTE_ETH_FOREACH_DEV(port_idx) {
        log_port_info(port_idx);
    }

    /*
     * rte_eal_init returns the number of parsed arguments; warn if some arguments were
     * unparsed.  We subtract two to account for the trailing null and the program name.
     */
    if (ret != static_cast<int>(dpdk_args.size() - 2)) {
        icp_log(ICP_LOG_ERROR, "DPDK initialization routine only parsed %d of %" PRIu64 " arguments\n",
                ret, dpdk_args.size() - 2);
    } else {
        icp_log(ICP_LOG_DEBUG, "DPDK initialized\n");
    }
}

class eal : public icp::packetio::component::registrar<eal> {
public:
    eal() { init(); }

    ~eal()
    {
        /* Shut up clang's warning about this being an unstable ABI function */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        rte_eal_cleanup();
#pragma clang diagnostic pop
    };

private:
    constexpr static int priority = 10;
};

}
}
}

extern "C" {

using namespace icp::packetio::driver;

int dpdk_option_init(void *opt_data __attribute__((unused)))
{
    /* Arguments need to start with the program name */
    auto& parser = arg_parser::instance();
    return (parser.init("icp_eal"));
}

int dpdk_option_handler(int opt, const char *opt_arg,
                       void *opt_data __attribute__((unused)))
{
    auto& parser = arg_parser::instance();
    return (parser.parse(opt, opt_arg));
}

}
