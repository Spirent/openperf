#include <cerrno>
#include <cstdlib>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <net/if.h>
#include <sys/capability.h>

#include "tl/expected.hpp"

#include "core/icp_core.h"
#include "drivers/dpdk/dpdk.h"
#include "drivers/dpdk/eal.h"
#include "drivers/dpdk/model/physical_port.h"
#include "packetio/generic_port.h"

namespace icp {
namespace packetio {
namespace dpdk {

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

static void log_port(uint16_t port_idx, struct rte_eth_dev_info &info)
{
    struct ether_addr mac_addr;
    rte_eth_macaddr_get(port_idx, &mac_addr);

    if (info.if_index > 0) {
        char if_name[IF_NAMESIZE];
        if_indextoname(info.if_index, if_name);
        icp_log(ICP_LOG_DEBUG, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s) attached to %s\n",
                port_idx,
                mac_addr.addr_bytes[0],
                mac_addr.addr_bytes[1],
                mac_addr.addr_bytes[2],
                mac_addr.addr_bytes[3],
                mac_addr.addr_bytes[4],
                mac_addr.addr_bytes[5],
                info.driver_name, if_name);
    } else {
        icp_log(ICP_LOG_DEBUG, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s)\n",
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

eal::eal(std::vector<std::string> args)
    : m_initialized(false)
{
    /* Check to see if we have permissions to launch DPDK */
    if (!sufficient_permissions()) {
        throw std::runtime_error("Insufficient permissions to initialize DPDK");
    }

    /* Convert args to c-strings for DPDK consumption */
    std::vector<char *> eal_args;
    eal_args.reserve(args.size() + 1);
    std::transform(begin(args), end(args), std::back_inserter(eal_args),
                   [](std::string &s) { return s.data(); });
    eal_args.push_back(nullptr); /* null terminator */

    icp_log(ICP_LOG_DEBUG, "Initializing DPDK with \"%s\"\n",
            std::accumulate(begin(args), end(args), std::string(),
                            [](const std::string &a, const std::string &b) -> std::string {
                                return a + (a.length() > 0 ? " " : "") + b;
                            }).c_str());

    int parsed_or_err = rte_eal_init(eal_args.size() - 1, eal_args.data());
    if (parsed_or_err < 0) {
        throw std::runtime_error("Failed to initialize DPDK");
    }

    m_initialized = true;

    /*
     * rte_eal_init returns the number of parsed arguments; warn if some arguments were
     * unparsed.  We subtract two to account for the trailing null and the program name.
     */
    if (parsed_or_err != static_cast<int>(eal_args.size() - 2)) {
        icp_log(ICP_LOG_ERROR, "DPDK initialization routine only parsed %d of %" PRIu64 " arguments\n",
                parsed_or_err, eal_args.size() - 2);
    } else {
        icp_log(ICP_LOG_DEBUG, "DPDK initialized\n");
    }

    /*
     * Loop through our available ports and...
     * 1. Log the MAC/driver to the console
     * 2. Generate a port_info vector
     */
    uint16_t port_idx = 0;
    std::vector<model::port_info> port_info;
    RTE_ETH_FOREACH_DEV(port_idx) {
        struct rte_eth_dev_info info;
        rte_eth_dev_info_get(port_idx, &info);
        log_port(port_idx, info);
        port_info.emplace_back(model::port_info(info.driver_name));
    }

    /* Use the port_info vector to allocate our default memory pools */
    m_allocator = std::make_unique<pool_allocator>(pool_allocator(port_info));

    /* Now use a pool from the pool allocator to configure our ports */
    port_idx = 0;
    RTE_ETH_FOREACH_DEV(port_idx) {
        auto port = model::physical_port(port_idx, m_allocator->rx_mempool());
        auto result = port.low_level_config();
        if (!result) {
            throw std::runtime_error(result.error());
        }
    }
}

eal::~eal()
{
    if (!m_initialized) {
        return;
    }

    /* Shut up clang's warning about this being an unstable ABI function */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    rte_eal_cleanup();
#pragma clang diagnostic pop
};

std::vector<int> eal::port_ids()
{
    uint16_t port_idx = 0;
    std::vector<int> port_ids;
    RTE_ETH_FOREACH_DEV(port_idx) {
        port_ids.emplace_back(port_idx);
    }

    return (port_ids);
}

std::optional<port::generic_port> eal::port(int id) const
{
    return (rte_eth_dev_is_valid_port(id)
            ? std::make_optional(port::generic_port(model::physical_port(id, m_allocator->rx_mempool())))
            : std::nullopt);
}

tl::expected<int, std::string> eal::create_port(const port::config_data& config)
{
    static int idx = 0;

    /* Sanity check input */
    if (!std::holds_alternative<port::bond_config>(config)) {
        return tl::make_unexpected("Missing bond configuration data");
    }

    /* Make sure that all ports in the vector actually exist */
    for (auto id : std::get<port::bond_config>(config).ports) {
        if (!rte_eth_dev_is_valid_port(id)) {
            return tl::make_unexpected("Port id " + std::to_string(id) + " is invalid");
        }
    }

    /* Well all right.  Let's create a port, shall we? */
    /*
     * XXX: DPDK uses the prefix of the name to find the proper driver;
     * ergo, this name cannot change.
     */
    std::string name = "net_bonding" + std::to_string(idx++);
    /**
     * XXX: The 3rd parameter _should_ be SOCKET_ID_ANY, since we don't care
     * about NUMA yet, however, the DPDK function signature's are screwed up.
     * This create function takes a uint8_t for the socket id, but the rest of
     * the DPDK code uses an int for the socket id.
     * So... we have to use 0 here so that we don't attempt to allocate memory
     * from NUMA socket 255.
     */
    int id_or_error = rte_eth_bond_create(name.c_str(),
                                          BONDING_MODE_8023AD,
                                          0);
    if (id_or_error < 0) {
        return tl::make_unexpected("Failed to create bond port: "
                                   + std::string(rte_strerror(std::abs(id_or_error))));
    }

    std::vector<int> success_record;
    for(auto id : std::get<port::bond_config>(config).ports) {
        int error = rte_eth_bond_slave_add(id_or_error, id);
        if (error) {
            for (auto added_id : success_record) {
                rte_eth_bond_slave_remove(id_or_error, added_id);
            }
            rte_eth_bond_free(name.c_str());
            return tl::make_unexpected("Failed to add slave port " + std::to_string(id)
                                       + "to bond port " + std::to_string(id_or_error)
                                       + ": " + std::string(rte_strerror(std::abs(error))));
        }
        success_record.push_back(id);
    }

    m_bond_ports[id_or_error] = name;
    return (id_or_error);
}

tl::expected<void, std::string> eal::delete_port(int id)
{
    if (!rte_eth_dev_is_valid_port(id)) {
        /* Deleting a non-existent port is fine */
        return {};
    }

    /* Port exists */
    if (m_bond_ports.find(id) == m_bond_ports.end()) {
        /* but it's not one we can delete */
        return tl::unexpected("Port " + std::to_string(id) + " cannot be deleted");
    }

    /*
     * There is apparently no way to query the number of slaves a port has,
     * so resort to brute force here.
     */
    uint16_t slaves[RTE_MAX_ETHPORTS] = {};
    int length_or_err = rte_eth_bond_slaves_get(id, slaves, RTE_MAX_ETHPORTS);
    if (length_or_err < 0) {
        /* Not sure what else we can do here... */
        icp_log(ICP_LOG_ERROR, "Could not retrieve slave port ids from bonded port %d\n", id);
    } else if (length_or_err > 0) {
        for (int i = 0; i < length_or_err; i++) {
            rte_eth_bond_slave_remove(id, i);
        }
    }
    rte_eth_bond_free(m_bond_ports[id].c_str());
    m_bond_ports.erase(id);
    return {};
}

}
}
}
