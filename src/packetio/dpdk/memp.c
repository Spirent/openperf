#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

/* Make sure we include everything we need for size calculation required by memp_std.h */
#include "lwip/pbuf.h"
#include "lwip/raw.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/ip4_frag.h"
#include "lwip/netbuf.h"
#include "lwip/api.h"
#include "lwip/priv/tcpip_priv.h"
#include "lwip/priv/api_msg.h"
#include "lwip/sockets.h"
#include "lwip/netifapi.h"
#include "lwip/etharp.h"
#include "lwip/igmp.h"
#include "lwip/timeouts.h"
/* needed by default MEMP_NUM_SYS_TIMEOUT */
#include "netif/ppp/ppp_opts.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/priv/nd6_priv.h"
#include "lwip/ip6_frag.h"
#include "lwip/mld6.h"

#include "packetio_dpdk.h"

/* Compile time sanity checks */
#if !MEM_USE_POOLS
#error "MEM_USE_POOLS is required in lwipopts.h"
#endif
#if MEMP_OVERFLOW_CHECK
#warn "MEMP_OVERFLOW_CHECK is ignored"
#endif

#define LWIP_MEMPOOL(name,num,size,desc) LWIP_MEMPOOL_DECLARE(name,num,size,desc)
#include "lwip/priv/memp_std.h"

const struct memp_desc * const memp_pools[MEMP_MAX] = {
#define LWIP_MEMPOOL(name, num, size, desc) &memp_ ## name,
#include "lwip/priv/memp_std.h"
};

static inline struct rte_mempool * _get_pool(const struct memp_desc *desc)
{
    return ((struct rte_mempool *)(desc->base));
}

#define dpdk_pool(desc) (struct rte_mempool *)(desc->base)

void memp_init_pool(const struct memp_desc *desc)
{
    unsigned nb_elements = rte_align32pow2(desc->num);
    struct rte_mempool *mp = rte_mempool_create(desc->desc,
                                                nb_elements,
                                                desc->size,
                                                0, 0,
                                                NULL, NULL,
                                                NULL, NULL,
                                                SOCKET_ID_ANY,
                                                MEMPOOL_F_NO_IOVA_CONTIG);
    LWIP_ASSERT("memp_init_pool: create failed", mp != NULL);

    struct memp_desc *d = (struct memp_desc *)(desc);
    d->num = nb_elements;
    d->base = (u8_t *)mp;
    d->tab = NULL;

#if MEMP_STATS
    desc->stats->avail = nb_elements;
    desc->stats->name = desc->desc;
#endif
}

void memp_init()
{
    for (unsigned i = 0; i < LWIP_ARRAYSIZE(memp_pools); i++) {
        memp_init_pool(memp_pools[i]);

#if LWIP_STATS && MEMP_STATS
        lwip_stats.memp[i] = memp_pools[i]->stats;
#endif
    }
}

static void * do_memp_malloc_pool(const struct memp_desc *desc)
{
    void *to_return = NULL;
    if (rte_mempool_get(_get_pool(desc), &to_return) != 0) {
        LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("memp_malloc: out of memory in pool %s\n", desc->desc));
        desc->stats->err++;
    } else {
        desc->stats->max = RTE_MAX(++desc->stats->used, desc->stats->max);
    }

    return (to_return);
}

void * memp_malloc(memp_t type)
{
    return (do_memp_malloc_pool(memp_pools[type]));
}

static void do_memp_free_pool(const struct memp_desc *desc, void *mem)
{
    rte_mempool_put(_get_pool(desc), mem);
    desc->stats->used--;
}

void memp_free(memp_t type, void *mem)
{
    do_memp_free_pool(memp_pools[type], mem);
}
