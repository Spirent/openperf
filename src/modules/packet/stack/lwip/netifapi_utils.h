#ifndef _NETIFAPI_UTILS_H_
#define _NETIFAPI_UTILS_H_

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct netif;

/**
 * Bring netif down safely.
 * 
 * The normal netif_set_down() function flushes the ARP tables but doesn't stop PCBs
 * created for the interface which are still active.
 * 
 * The PCBs are only stopped when netif_remove() is called.
 * 
 * This can cause issues if PCBs have timers which trigger ARP requests when
 * the interface is not configured.  The stack will try to send out a packet
 * when the interface doesn't have a MAC which can cause a fatal error.
 */
void netifapi_netif_safe_set_down(struct netif *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_NETIF_API */

#endif /* _NETIFAPI_UTILS_H_ */
