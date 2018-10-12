#include <memory>
#include <numeric>

#include "core/icp_core.h"
#include "drivers/dpdk/dpdk.h"
#include "drivers/dpdk/model/port_info.h"
#include "memory/dpdk/memp.h"
#include "memory/dpdk/pool_allocator.h"

namespace icp {
namespace packetio {
namespace dpdk {

static const uint32_t MBUF_LENGTH_ALIGN = 64;

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
    for (size_t i = 0; i < icp_count_of(packetio_cache_size_map); i++) {
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
    return (rte_is_power_of_2(nb_mbufs)
            ? nb_mbufs - 1
            : rte_align32pow2(nb_mbufs) - 1);
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

static void log_mempool(const struct rte_mempool *mpool)
{
    icp_log(ICP_LOG_DEBUG, "%s: %u bytes, %u of %u mbufs available\n",
            mpool->name,
            rte_pktmbuf_data_room_size((struct rte_mempool *)mpool),
            rte_mempool_avail_count(mpool), mpool->size);
}

static rte_mempool* _create_pbuf_mempool(const char* name, size_t size,
                                         bool cached, bool direct)
{
    size_t nb_mbufs = icp_min(131072, _pool_size_adjust(icp_max(1024U, size)));

    rte_mempool* mp = rte_pktmbuf_pool_create(
        name,
        nb_mbufs,
        cached ? _get_cache_size(nb_mbufs) : 0,
        RTE_ALIGN(sizeof(struct pbuf), RTE_MBUF_PRIV_ALIGN),
        direct ? _mbuf_length_adjust(PBUF_POOL_BUFSIZE) : 0,
        SOCKET_ID_ANY);

    if (!mp) {
        throw std::runtime_error(std::string("Could not allocate mempool = ") + name);
    }

    log_mempool(mp);

    return (mp);
}

pool_allocator::pool_allocator(std::vector<model::port_info> &info)
    : ref_rom_pool(_create_pbuf_mempool(memp_ref_rom_mempool, MEMP_NUM_PBUF, false, false))
{
    /* Base default pool size on the number and types of ports */
    default_pool.reset(_create_pbuf_mempool(memp_default_mempool,
                                            std::accumulate(begin(info), end(info), 0,
                                                            [](int lhs, const model::port_info &rhs) {
                                                                return (lhs
                                                                        + rhs.rx_desc_count()
                                                                        + rhs.tx_desc_count());
                                                            }),
                                            true, true));
};

}
}
}
