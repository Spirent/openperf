#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#include "lwip/def.h"
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

#include "drivers/dpdk/dpdk.h"
#include "memory/dpdk/memp.h"

/* Compile time sanity checks */
#if MEMP_OVERFLOW_CHECK
#warn "MEMP_OVERFLOW_CHECK is ignored."
#endif

#if MEMP_USE_POOLS
#error "MEMP_MEM_POOLS is not supported; FIX ME if you want."
#endif

extern struct rte_mbuf * packetio_memp_pbuf_to_mbuf(const struct pbuf *pbuf);
extern struct pbuf * packetio_memp_mbuf_to_pbuf(const struct rte_mbuf *mbuf);

const char memp_default_mempool[] = "DEFAULT";
const char memp_ref_rom_mempool[] = "REF_ROM";

#define LWIP_MEMPOOL(name,num,size,desc) LWIP_MEMPOOL_DECLARE(name,num,size,desc)
#include "lwip/priv/memp_std.h"

const struct memp_desc * const memp_pools[MEMP_MAX] = {
#define LWIP_MEMPOOL(name, num, size, desc) &memp_ ## name,
#include "lwip/priv/memp_std.h"
};

static struct rte_mempool* get_default_mempool()
{
    static atomic_flag have_mp = ATOMIC_FLAG_INIT;
    static struct rte_mempool* default_mp = NULL;
    if (!atomic_flag_test_and_set(&have_mp)) {
        default_mp = rte_mempool_lookup(memp_default_mempool);
    }
    assert(default_mp);
    return (default_mp);
}

static struct rte_mempool* get_ref_rom_mempool()
{
    static atomic_flag have_mp = ATOMIC_FLAG_INIT;
    static struct rte_mempool* ref_rom_mp = NULL;
    if (!atomic_flag_test_and_set(&have_mp)) {
        ref_rom_mp = rte_mempool_lookup(memp_ref_rom_mempool);
    }
    assert(ref_rom_mp);
    return (ref_rom_mp);
}

void memp_init()
{
    get_default_mempool();
    get_ref_rom_mempool();

    /* Initialize stats, maybe */
    for (unsigned i = 0; i < LWIP_ARRAYSIZE(memp_pools); i++) {
#if LWIP_STATS && MEMP_STATS
        memp_pools[i]->stats->name = memp_pools[i]->desc;
        lwip_stats.memp[i] = memp_pools[i]->stats;
#endif
    }
}

void * memp_malloc(memp_t type)
{
    void *to_return = NULL;
    struct rte_mbuf *mbuf = NULL;
    switch (type) {
    case MEMP_PBUF:
        mbuf = rte_pktmbuf_alloc(get_ref_rom_mempool());
        if (mbuf) {
            to_return = packetio_memp_mbuf_to_pbuf(mbuf);
        }
        break;
    case MEMP_PBUF_POOL:
        mbuf = rte_pktmbuf_alloc(get_default_mempool());
        if (mbuf) {
            to_return = packetio_memp_mbuf_to_pbuf(mbuf);
        }
        break;
    default:
        to_return = rte_malloc(memp_pools[type]->desc, memp_pools[type]->size, 0);

#if MEMP_STATS
        {
            const struct memp_desc * const desc = memp_pools[type];
            if (to_return) {
                desc->stats->max = icp_max(++desc->stats->used, desc->stats->max);
            } else {
                LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                            ("memp_malloc: out of memory in pool %s\n", desc->desc));
                desc->stats->err++;
            }
        }
#endif
    }

    return (to_return);
}

void memp_free(memp_t type, void *mem)
{
    switch (type) {
    case MEMP_PBUF:
    case MEMP_PBUF_POOL:
        rte_pktmbuf_free_seg(packetio_memp_pbuf_to_mbuf((struct pbuf *)mem));
        break;
    default:
        rte_free(mem);

#if MEMP_STATS
        {
            const struct memp_desc * const desc = memp_pools[type];
            LWIP_ASSERT("free before alloc?", desc->stats->used != 0);
            desc->stats->used--;
        }
#endif
    }
}
