#include <memory>
#include <numeric>

#include "lwip/pbuf.h"

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/memory/dpdk/memp.h"
#include "packetio/memory/dpdk/pool_allocator.hpp"

namespace openperf {
namespace packetio {
namespace dpdk {

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

#if RTE_MBUF_PRIV_ALIGN != MEM_ALIGNMENT
#error "RTE_MBUF_PRIV_ALIGN and lwip's MEM_ALIGNMENT msut be equal"
#endif

static struct cache_size_map
{
    uint32_t nb_mbufs;
    uint32_t cache_size;
} packetio_cache_size_map[] = {
    {63, 9},       /* 63 % 9 == 0 */
    {127, 21},     /* 127 % 21 == 1 (127 is prime) */
    {255, 51},     /* 255 % 51 == 0 */
    {511, 73},     /* 511 % 73 == 0 */
    {1023, 341},   /* 1023 % 341 == 0 */
    {2047, 341},   /* 2047 % 341 == 1 */
    {4095, 455},   /* 4095 % 455 == 0 */
    {8191, 455},   /* 8191 % 455 == 1 (8191 is prime) */
    {16383, 455},  /* 16383 % 455 == 3 */
    {32767, 504},  /* 32767 % 504 == 7 */
    {65535, 508},  /* 65535 % 508 == 3 */
    {131071, 510}, /* 131071 % 510 == 1 (131071 is prime) */
};

__attribute__((const)) static uint32_t get_cache_size(uint32_t nb_mbufs)
{
    for (size_t i = 0; i < op_count_of(packetio_cache_size_map); i++) {
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
__attribute__((const)) static uint32_t pool_size_adjust(uint32_t nb_mbufs)
{
    return (rte_is_power_of_2(nb_mbufs) ? nb_mbufs - 1
                                        : rte_align32pow2(nb_mbufs) - 1);
}

static void log_mempool(const struct rte_mempool* mpool)
{
    OP_LOG(OP_LOG_DEBUG,
           "%s: %u, %u byte mbufs on NUMA socket %d\n",
           mpool->name,
           mpool->size,
           rte_pktmbuf_data_room_size((struct rte_mempool*)mpool),
           mpool->socket_id);
}

static rte_mempool* create_pbuf_mempool(
    const char* name, size_t size, bool cached, bool direct, int socket_id)
{
    static_assert(PBUF_PRIVATE_SIZE >= sizeof(struct pbuf));

    size_t nb_mbufs = op_min(131072, pool_size_adjust(op_max(1024U, size)));

    rte_mempool* mp =
        rte_pktmbuf_pool_create_by_ops(name,
                                       nb_mbufs,
                                       cached ? get_cache_size(nb_mbufs) : 0,
                                       PBUF_PRIVATE_SIZE,
                                       direct ? PBUF_POOL_BUFSIZE : 0,
                                       socket_id,
                                       "stack");

    if (!mp) {
        throw std::runtime_error(std::string("Could not allocate mempool = ")
                                 + name);
    }

    log_mempool(mp);

    return (mp);
}

pool_allocator::pool_allocator(const std::vector<model::port_info>& info,
                               const std::map<int, queue::count>& q_counts)
{
    /* Base default pool size on the number and types of ports on each NUMA node
     */
    for (auto i = 0U; i < RTE_MAX_NUMA_NODES; i++) {
        auto sum = std::accumulate(
            begin(info),
            end(info),
            0,
            [&](unsigned lhs, const model::port_info& rhs) {
                return (rhs.socket_id() == i ? (
                            lhs
                            + (q_counts.at(rhs.id()).rx * rhs.rx_desc_count())
                            + (q_counts.at(rhs.id()).tx * rhs.tx_desc_count()))
                                             : lhs);
            });
        if (sum) {
            /*
             * Create both a ref/rom and default mempool for each NUMA node with
             * ports Note: we only use a map for the rom/ref pools for symmetry.
             * We never actually need to look them up.
             */
            std::array<char, RTE_MEMPOOL_NAMESIZE> name_buf;
            snprintf(name_buf.data(),
                     RTE_MEMPOOL_NAMESIZE,
                     memp_ref_rom_mempool_fmt,
                     i);
            m_ref_rom_mpools.emplace(
                i,
                create_pbuf_mempool(
                    name_buf.data(), MEMP_NUM_PBUF, false, false, i));

            snprintf(name_buf.data(),
                     RTE_MEMPOOL_NAMESIZE,
                     memp_default_mempool_fmt,
                     i);
            m_default_mpools.emplace(
                i, create_pbuf_mempool(name_buf.data(), sum, true, true, i));
        }
    }
};

rte_mempool* pool_allocator::rx_mempool(unsigned socket_id) const
{
    assert(socket_id <= RTE_MAX_NUMA_NODES);
    auto found = m_default_mpools.find(socket_id);
    return (found == m_default_mpools.end() ? nullptr : found->second.get());
}

} // namespace dpdk
} // namespace packetio
} // namespace openperf
