#ifndef _IPV6_ND6_OP_H_
#define _IPV6_ND6_OP_H_

#include "lwip/opt.h"

#if LWIP_IPV6

#include "lwip/ip6_addr.h"
#include "lwip/err.h"
#include "lwip/prot/ethernet.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

s8_t nd6_get_next_hop_entry(const ip6_addr_t *ip6addr, struct netif *netif);

int
neighbor_cache_get_entry(size_t i, ip6_addr_t **ipaddr, u8_t **lladdr_ret);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_IPV6 */

#endif /* _IPV6_ND6_OP_H_ */
