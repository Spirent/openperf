#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#include "lwip/etharp.h"
#include "lwip/netifapi.h"
#include "lwip/memp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/priv/raw_priv.h"
#include "lwip/udp.h"

#include "packet/stack/lwip/netifapi_utils.h"

/**
 * Notify stack that interface IP address changed
 * 
 * This function is from lwip src/core/netif.c
 * Need to duplicate here because it is defined as static
 */
static void netif_do_ip_addr_changed(const ip_addr_t* old_addr,
                                     const ip_addr_t* new_addr)
{
#if LWIP_TCP
    tcp_netif_ip_addr_changed(old_addr, new_addr);
#endif /* LWIP_TCP */
#if LWIP_UDP
    udp_netif_ip_addr_changed(old_addr, new_addr);
#endif /* LWIP_UDP */
#if LWIP_RAW
    raw_netif_ip_addr_changed(old_addr, new_addr);
#endif /* LWIP_RAW */
}

/**
 * Abort any PCBs for addresses used by the interface.
 * 
 * This logic is from lwip netif_remove() in src/core/netif.c
 */
static void netif_abort_pcbs(struct netif* netif)
{
#if LWIP_IPV4
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif))) {
        netif_do_ip_addr_changed(netif_ip_addr4(netif), NULL);
    }
#endif /* LWIP_IPV4*/

#if LWIP_IPV6
    for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
        if (ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
            netif_do_ip_addr_changed(netif_ip_addr6(netif, i), NULL);
        }
    }
#endif /* LWIP_IPV6 */
}

static void netif_safe_set_down(struct netif* netif)
{
    netif_abort_pcbs(netif);
    netif_set_down(netif);
}

void netifapi_netif_safe_set_down(struct netif* netif)
{
    netifapi_netif_common(netif, netif_safe_set_down, NULL);
}

#endif /* LWIP_NETIF_API */
