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

s8_t nd6_get_next_hop_entry_probe(const ip6_addr_t *ip6addr, struct netif *netif, int probe);

int
neighbor_cache_get_entry(size_t i, ip6_addr_t **ipaddr, u8_t **lladdr_ret, u8_t* state);

/*
 * lwip only supports IPv6 prefix length /64 and does not support default gateway address.
 *
 * The code below adds basic support for both of these features.
 * The IPv6 prefix length and gateway address are stored in the net_interface class.
 * Ideally these would be added to the netif struct.
 */

/**
 * @ingroup netif
 * Compare if the addresses have the same prefix.
 * @param addr1 pointer to first IPv6 address
 * @param addr2 pointer to second IPv6 address
 * @param prefix_len The IPv6 network prefix length.
 */
static inline int ip6_addr_netcmp_prefix_zoneless(const ip6_addr_t *addr1, const ip6_addr_t *addr2, u8_t prefix_len)
{
  const u8_t *a1 = (const u8_t*)addr1;
  const u8_t *a2 = (const u8_t*)addr2;
  ldiv_t r;
  u8_t mask;

  if (prefix_len > 128) prefix_len = 128;
  r = ldiv(prefix_len, 8);
  if (r.quot && memcmp(a1, a2, r.quot) != 0) return 0;
  if (!r.rem) return 1;
  mask = (0xff << (8 - r.rem));
  return (a1[r.quot] & mask) == (a2[r.quot] & mask);
}

#define ip6_addr_netcmp_prefix(addr1, addr2, prefix_len) \
    (ip6_addr_netcmp_prefix_zoneless((addr1), (addr2), prefix_len) && \
                                     ip6_addr_cmp_zone((addr1), (addr2)))

u8_t netif_ip6_prefix_len(struct netif *netif, int idx);
void netif_set_ip6_prefix_len(struct netif *netif, int idx, u8_t prefix_len);

const ip6_addr_t * netif_ip6_gateway_addr(struct netif *netif);
void netif_set_ip6_gateway_addr(struct netif *netif, const ip6_addr_t *addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWIP_IPV6 */

#endif /* _IPV6_ND6_OP_H_ */
