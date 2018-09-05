#include <stdbool.h>

#include <net/if.h>
#include <sys/capability.h>

#include "lwip/init.h"

#include "core/icp_core.h"
#include "packetio_dpdk.h"
#include "packetio_stack.h"

struct named_cap_flag {
    const char *name;
    cap_flag_t flag;
};

static struct named_cap_flag permissions[] = {
    {"cap_ipc_local", CAP_IPC_LOCK},
    {"cap_net_raw", CAP_NET_RAW},
};

static bool _dpdk_perm_ok()
{
    cap_t caps = cap_get_proc();
    if (!caps) {
        icp_log(ICP_LOG_ERROR, "Could not retrieve any capabilities.\n");
        return (false);
    }

    size_t nb_permissions = sizeof(permissions) / sizeof(struct named_cap_flag);
    bool have_permissions = true;  /* assume we have permission unless we don't */
    for (size_t i = 0; i < nb_permissions; i++) {
        cap_flag_value_t flag;
        if (cap_get_flag(caps, permissions[i].flag, CAP_EFFECTIVE, &flag) == -1) {
            icp_log(ICP_LOG_ERROR, "cap_get_flag returned error for %s.\n",
                    permissions[i].name);
            have_permissions = false;
            break;
        } else if (!flag) {
            icp_log(ICP_LOG_INFO, "Missing required DPDK capability: %s\n",
                    permissions[i].name);
            have_permissions = false;
        }
    }

    cap_free(caps);

    return (have_permissions);
}

static void _log_dpdk_port_info(uint8_t port)
{
    struct rte_eth_dev_info info;
    rte_eth_dev_info_get(port, &info);

    struct ether_addr mac_addr;
    rte_eth_macaddr_get(port, &mac_addr);

    if (info.if_index > 0) {
        char if_name[IF_NAMESIZE];
        if_indextoname(info.if_index, if_name);
        icp_log(ICP_LOG_INFO, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s) attached to %s\n",
                port,
                mac_addr.addr_bytes[0],
                mac_addr.addr_bytes[1],
                mac_addr.addr_bytes[2],
                mac_addr.addr_bytes[3],
                mac_addr.addr_bytes[4],
                mac_addr.addr_bytes[5],
                info.driver_name, if_name);
    } else {
        icp_log(ICP_LOG_INFO, "  dpdk%d: %02x:%02x:%02x:%02x:%02x:%02x (%s)\n",
                port,
                mac_addr.addr_bytes[0],
                mac_addr.addr_bytes[1],
                mac_addr.addr_bytes[2],
                mac_addr.addr_bytes[3],
                mac_addr.addr_bytes[4],
                mac_addr.addr_bytes[5],
                info.driver_name);
    }
}

int _get_waiting_lcore()
{
    size_t i = 0;
    RTE_LCORE_FOREACH_SLAVE(i) {
        switch (rte_eal_get_lcore_state(i)) {
        case FINISHED:
            rte_eal_wait_lcore(i);
        case WAIT:
            return ((int)i);
        default:
            continue;
        }
    }
    return (-1);
}

extern struct rte_mempool *packetio_memp_pool();

int packetio_init(int argc, char * argv[])
{
    /* Check to see if we have permission to run DPDK; if not bail */
    if (!_dpdk_perm_ok()) {
        icp_exit("Insufficient permissions to load DPDK");
    }

    rte_log_set_global_level(8);
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        icp_exit("Failed to initialize DPDK: %s\n", rte_strerror(rte_errno));
    }

    /* Log port info */
    unsigned nb_ports = rte_eth_dev_count_avail();
    if (!nb_ports) {
        icp_exit("No DPDK ports available!\n");
    }
    icp_log(ICP_LOG_INFO, "Found %d ports via %s\n", nb_ports, rte_version());
    unsigned i = 0;
    RTE_ETH_FOREACH_DEV(i) {
        _log_dpdk_port_info(i);
    }

    /* Initialize stack */
    lwip_init();

    //stats_display();

    /* Now configure and bind ports to the stack */
    /* Just 1 for now */
    int lcore = _get_waiting_lcore();
    if (lcore < 0) {
        icp_log(ICP_LOG_ERROR, "No DPDK cores available for running stack\n");
        return (-EBUSY);
    }

    /* Launch the stack on a lcore */
    struct stack_args *args = calloc(1, sizeof(*args));
    if (!args) {
        return (-ENOMEM);
    }
    args->pool = packetio_memp_pool();
    args->port_id = 0;

    return (rte_eal_remote_launch(packetio_stack_run, args, lcore));
}
