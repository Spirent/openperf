#include "lwip/etharp.h"
#include "lwip/netif.h"
#include "lwip/snmp.h"
#include "lwip/ip_addr.h"

#include "core/icp_core.h"
#include "packetio_dpdk.h"
#include "packetio_memp.h"

#define IFNAME0 'i'
#define IFNAME1 'o'

extern void packetio_memp_check_pools();

static err_t packetio_netif_tx(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    if (!p) {
        return (ERR_OK);
    }
    struct rte_mbuf *pkts[] = { packetio_memp_pbuf_to_mbuf(p) };
    int tx = rte_eth_tx_burst(0, 0, pkts, 1);
    return (tx == 1 ? ERR_OK : ERR_BUF);
}

err_t packetio_netif_init(struct netif *netif)
{
    /* Initialize the snmp variables and counters inside the struct netif */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmad,
                    packetio_port_speed(0));

    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

#if LWIP_IPV4
    netif->output = etharp_output;
#endif
    netif->linkoutput = packetio_netif_tx;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    rte_eth_macaddr_get(0, (struct ether_addr *)&netif->hwaddr);

    netif->mtu = 1500;

    netif->flags = (NETIF_FLAG_BROADCAST
                    | NETIF_FLAG_ETHARP
                    | NETIF_FLAG_ETHERNET
                    | NETIF_FLAG_IGMP
                    | NETIF_FLAG_MLD6
                    | NETIF_FLAG_LINK_UP);

#if LWIP_IPV6 && LWIP_IPV6_MLD
    /* IPv6 setup */
#endif

    return ERR_OK;
}
