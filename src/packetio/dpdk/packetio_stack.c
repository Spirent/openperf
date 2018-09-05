#include <unistd.h>

#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

#include "core/icp_core.h"
#include "packetio_dpdk.h"
#include "packetio_netif.h"
#include "packetio_pbuf.h"
#include "packetio_port.h"
#include "packetio_stack.h"

#define PKT_BURST_SIZE 32

static int packetio_stack_main(struct netif *netif, struct stack_args *args)
{
    int ret = 0;
    while (!packetio_port_link_check(args->port_id)) {
        rte_pause();
    }

    netif_set_link_up(netif);
    dhcp_start(netif);

    for (;;) {
        sys_check_timeouts();

        struct rte_mbuf *packets[PKT_BURST_SIZE];
        unsigned nb_pkts = rte_eth_rx_burst(args->port_id, 0, packets, PKT_BURST_SIZE);

        for (unsigned i = 0; i < nb_pkts; i++) {
            netif->input(packetio_pbuf_synchronize(packets[i]), netif);
        }
    }

    return (ret);
}

static int packetio_stack_lsc_callback(uint16_t port_idx,
                                       enum rte_eth_event_type type __attribute__((unused)),
                                       void *param,
                                       void *ret_param __attribute__((unused)))
{
    struct netif *netif = param;
    packetio_port_link_log(port_idx);
    return (0);
}

int packetio_stack_run(void *vargs)
{
    struct stack_args *args = vargs;

    /* Configure underlying port */
    int error = packetio_port_configure(args->port_id, 1, 1, args->pool);
    if (error) {
        icp_log(ICP_LOG_ERROR, "Could not configure port %d: %s\n",
                args->port_id, strerror(abs(error)));
        return (error);
    }

    /* Configure stack interface for port */
    struct netif stackif;
    struct netif *ifp = netif_add(&stackif,
                                  NULL, NULL, NULL,
                                  args,
                                  packetio_netif_init,
                                  ethernet_input);
    if (!ifp) {
        icp_log(ICP_LOG_ERROR, "Could not setup stack interface on port %d\n",
                args->port_id);
        return (-1);
    }

    icp_log(ICP_LOG_INFO, "Port speed = %d\n", packetio_port_speed(args->port_id));
    if (!packetio_port_config_lsc_interrupt(args->port_id)) {
        icp_log(ICP_LOG_WARNING, "Link status interrupt not supported on port %d\n",
                args->port_id);
    }
    error = rte_eth_dev_callback_register(args->port_id, RTE_ETH_EVENT_INTR_LSC,
                                          packetio_stack_lsc_callback, ifp);

    if (error) {
        icp_log(ICP_LOG_ERROR, "Could not register link statuc callback on port %d\n",
                args->port_id);
        return (-1);
    }

    netif_set_up(ifp);

    int ret = packetio_stack_main(ifp, args);

    free(args);

    return (ret);
}
