#ifndef _IPV6_NETIFAPI_H_
#define _IPV6_NETIFAPI_H_

#include "lwip/opt.h"

#if LWIP_NETIF_API /* don't build if not configured for use in lwipopts.h */
#if LWIP_IPV6

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

err_t netifapi_netif_create_ip6_linklocal_address(struct netif* netif,
                                                  u8_t from_mac_48bit);

err_t netifapi_netif_add_ip6_address(struct netif* netif,
                                     const ip6_addr_t* ipaddr,
                                     s8_t* chosen_idx);

err_t netifapi_netif_ip6_addr_set(struct netif* netif,
                                  s8_t addr_idx,
                                  const ip6_addr_t* ipaddr);

err_t netifapi_dhcp6_enable_stateless(struct netif* netif);

err_t netifapi_dhcp6_enable_stateful(struct netif* netif);

err_t netifapi_dhcp6_disable(struct netif* netif);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_IPV6 */
#endif /* LWIP_NETIF_API */

#endif /* _IPV6_NETIFAPI_H_ */
