#include <stdatomic.h>

#include "lwip/def.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/stats.h"

/* Make sure we include everything we need for size calculation required by
 * memp_std.h */
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

#include "core/op_common.h"

#include "packet/stack/lwip/memory.h"

/* Compile time sanity checks */
#if MEMP_OVERFLOW_CHECK
#warn "MEMP_OVERFLOW_CHECK is ignored."
#endif

#if MEMP_USE_POOLS
#error "MEMP_MEM_POOLS is not supported; FIX ME if you want."
#endif

#define LWIP_MEMPOOL(name, num, size, desc)                                    \
    LWIP_MEMPOOL_DECLARE(name, num, size, desc)
#include "lwip/priv/memp_std.h"

const struct memp_desc* const memp_pools[MEMP_MAX] = {
#define LWIP_MEMPOOL(name, num, size, desc) &memp_##name,
#include "lwip/priv/memp_std.h"
};

void memp_init()
{
    packet_stack_memp_initialize();

    /* Initialize stats, maybe */
    for (unsigned i = 0; i < LWIP_ARRAYSIZE(memp_pools); i++) {
#if LWIP_STATS && MEMP_STATS
        memp_pools[i]->stats->name = memp_pools[i]->desc;
        lwip_stats.memp[i] = memp_pools[i]->stats;
#endif
    }
}

void* memp_malloc(memp_t type)
{
#if MEMP_STATS
    const struct memp_desc* const desc = memp_pools[type];
#endif
    void* to_return = NULL;

    /*
     * Note: the DPDK memory pools maintain their own internal counters for
     * usage.  Additionally, drivers can allocate memory from these pools
     * directly.  As a result, any use/max counters we attempt to keep here
     * would be wrong.
     */
    switch (type) {
    case MEMP_PBUF:
    case MEMP_PBUF_POOL:
        to_return = packet_stack_pbuf_alloc();
        break;
    default:
        to_return = packet_stack_memp_alloc(memp_pools[type]->desc,
                                            memp_pools[type]->size);

#if MEMP_STATS
        if (to_return) {
            mem_size_t used = atomic_fetch_add_explicit(
                                  (_Atomic mem_size_t*)&desc->stats->used,
                                  1,
                                  memory_order_release)
                              + 1;
            desc->stats->max = op_max(used, desc->stats->max);
        }
#endif
    }

#if MEMP_STATS
    if (!to_return) {
        LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("memp_malloc: out of memory in pool %s\n", desc->desc));
        atomic_fetch_add_explicit(
            (_Atomic mem_size_t*)&desc->stats->err, 1, memory_order_release);
    }
#endif
    return (to_return);
}

void memp_free(memp_t type, void* mem)
{
    switch (type) {
    case MEMP_PBUF:
    case MEMP_PBUF_POOL:
        packet_stack_pbuf_free((struct pbuf*)mem);
        break;
    default:
        packet_stack_memp_free(mem);

#if MEMP_STATS
        {
            const struct memp_desc* const desc = memp_pools[type];
            mem_size_t used = atomic_fetch_sub_explicit(
                (_Atomic mem_size_t*)&desc->stats->used,
                1,
                memory_order_release);
            LWIP_ASSERT("free before alloc?", used != 0);
        }
#endif
    }
}
