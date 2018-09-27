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

#include "core/icp_core.h"
#include "drivers/dpdk/dpdk.h"
#include "memory/dpdk/memp.h"

/* Compile time sanity checks */
#if MEMP_OVERFLOW_CHECK
#warn "MEMP_OVERFLOW_CHECK is ignored."
#endif

#if MEMP_USE_POOLS
#error "MEMP_MEM_POOLS is not supported; FIX ME if you want."
#endif

#define LWIP_MEMPOOL(name,num,size,desc) LWIP_MEMPOOL_DECLARE(name,num,size,desc)
#include "lwip/priv/memp_std.h"

const struct memp_desc * const memp_pools[MEMP_MAX] = {
#define LWIP_MEMPOOL(name, num, size, desc) &memp_ ## name,
#include "lwip/priv/memp_std.h"
};

static const uint32_t MBUF_LENGTH_ALIGN = 64;

/* Pointer to packet pools */
static struct rte_mempool *_pbuf_mp = NULL;
static struct rte_mempool *_pbuf_pool_mp = NULL;

extern struct rte_mbuf * packetio_memp_pbuf_to_mbuf(const struct pbuf *pbuf);
extern struct pbuf * packetio_memp_mbuf_to_pbuf(const struct rte_mbuf *mbuf);

static size_t _dpdk_slave_count()
{
    size_t count = 0;
    unsigned i = 0;
    RTE_LCORE_FOREACH_SLAVE(i) {
        count += !!rte_lcore_is_enabled(i);
    }
    return (count);
}

/*
 * Per the DPDK documentation, cache size should be a divisor of pool
 * size.  However, since the optimal pool size is a Mersenne number,
 * nice multiples of 2 tend to be wasteful.  This table specifies the
 * cache size to use for various pool sizes in order to minimize wasted
 * pool elements.
 */

#if RTE_MEMPOOL_CACHE_MAX_SIZE < 512
#error "RTE_MEMPOOL_CACHE_MAX_SIZE must be at least 512"
#endif

static struct cache_size_map {
    uint32_t nb_mbufs;
    uint32_t cache_size;
} packetio_cache_size_map[] = {
    {     63,   9 },  /* 63 % 9 == 0 */
    {    127,  21 },  /* 127 % 21 == 1 (127 is prime) */
    {    255,  51 },  /* 255 % 51 == 0 */
    {    511,  73 },  /* 511 % 73 == 0 */
    {   1023, 341 },  /* 1023 % 341 == 0 */
    {   2047, 341 },  /* 2047 % 341 == 1 */
    {   4095, 455 },  /* 4095 % 455 == 0 */
    {   8191, 455 },  /* 8191 % 455 == 1 (8191 is prime) */
    {  16383, 455 },  /* 16383 % 455 == 3 */
    {  32767, 504 },  /* 32767 % 504 == 7 */
    {  65535, 508 },  /* 65535 % 508 == 3 */
    { 131071, 510 },  /* 131071 % 510 == 1 (131071 is prime) */
};

__attribute__((const))
static uint32_t _get_cache_size(uint32_t nb_mbufs)
{
    for (size_t i = 0; i < LWIP_ARRAYSIZE(packetio_cache_size_map); i++) {
        if (packetio_cache_size_map[i].nb_mbufs == nb_mbufs) {
            return (packetio_cache_size_map[i].cache_size);
        }
    }

    return (RTE_MEMPOOL_CACHE_MAX_SIZE);
}

/*
 * Convert nb_mbufs to a Mersenne number, as those are the
 * most efficient size for mempools.  If our input is already a
 * power of 2, return input - 1 instead of doubling the size.
 */
__attribute__((const))
static uint32_t _pool_size_adjust(uint32_t nb_mbufs)
{
    if (rte_is_power_of_2(nb_mbufs)) {
        return (nb_mbufs - 1);
    }

    return (rte_align32pow2(nb_mbufs) - 1);
}

/*
 * Adjust length to the next largest multiple of our alignment after
 * adding in DPDK overhead.
 */
__attribute__((const))
static uint32_t _mbuf_length_adjust(uint32_t length)
{
    return ((length + RTE_PKTMBUF_HEADROOM + (MBUF_LENGTH_ALIGN - 1)) & ~(MBUF_LENGTH_ALIGN - 1));
}

void _log_mbuf_pool_info(const struct rte_mempool *mpool)
{
    icp_log(ICP_LOG_DEBUG, "%s: %u bytes, %u of %u mbufs available\n",
            mpool->name,
            rte_pktmbuf_data_room_size((struct rte_mempool *)mpool),
            rte_mempool_avail_count(mpool), mpool->size);
}

void memp_init()
{
    /* Create and set default pbuf pool */
#if 0
    unsigned nb_mbufs = 1 + RTE_MEMPOOL_CACHE_MAX_SIZE * _dpdk_slave_count();
    uint16_t i = 0;
    RTE_ETH_FOREACH_DEV(i) {
        nb_mbufs += (packetio_port_config_rx_desc_count(i)
                     + packetio_port_config_tx_desc_count(i));
    }
#endif
    unsigned nb_mbufs = 65536;

    /* Adjust input for efficiency and overhead */
    nb_mbufs = icp_min(131072, _pool_size_adjust(icp_max(1024U, nb_mbufs)));

    _pbuf_pool_mp = rte_pktmbuf_pool_create_by_ops("PBUF_POOL",
                                                   nb_mbufs,
                                                   _get_cache_size(nb_mbufs),
                                                   RTE_ALIGN(sizeof(struct pbuf),
                                                             RTE_MBUF_PRIV_ALIGN),
                                                   _mbuf_length_adjust(PBUF_POOL_BUFSIZE),
                                                   SOCKET_ID_ANY,
                                                   "stack");
    if (!_pbuf_pool_mp) {
        icp_exit("Could not allocate default pbuf pool: %s", rte_strerror(rte_errno));
    }
    _log_mbuf_pool_info(_pbuf_pool_mp);

    /* Create and set default rom/ref pool */
    _pbuf_mp = rte_pktmbuf_pool_create_by_ops("PBUF_REF/ROM",
                                              MEMP_NUM_PBUF,
                                              0,
                                              RTE_ALIGN(sizeof(struct pbuf),
                                                        RTE_MBUF_PRIV_ALIGN),
                                              0,
                                              SOCKET_ID_ANY,
                                              "stack");
    if (!_pbuf_mp) {
        icp_exit("Could not allocate static pbuf pool: %s", rte_strerror(rte_errno));
    }
    _log_mbuf_pool_info(_pbuf_mp);

    /* Initialize stats, maybe */
    for (int i = 0; i < LWIP_ARRAYSIZE(memp_pools); i++) {
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
        mbuf = rte_pktmbuf_alloc(_pbuf_mp);
        if (mbuf) {
            to_return = packetio_memp_mbuf_to_pbuf(mbuf);
        }
        break;
    case MEMP_PBUF_POOL:
        mbuf = rte_pktmbuf_alloc(_pbuf_pool_mp);
        if (mbuf) {
            to_return = packetio_memp_mbuf_to_pbuf(mbuf);
        }
        break;
    default:
        to_return = malloc(memp_pools[type]->size);
    }

#if MEMP_STATS
    struct memp_desc *desc = memp_pools[type];
    if (to_return) {
        desc->stats->max = icp_max(++desc->stats->used, desc->stats->max);
    } else {
        LWIP_DEBUGF(MEMP_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("memp_malloc: out of memory in pool %s\n", desc->desc));
        desc->stats->err++;
    }
#endif

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
        free(mem);
    }

#if MEMP_STATS
    struct memp_desc *desc = memp_pools[type];
    desc->stats->used--;
#endif
}

struct rte_mempool *packetio_memp_pool()
{
    return (_pbuf_pool_mp);
}
