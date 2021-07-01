#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/mempool.hpp"
#include "packetio/memory/dpdk/primary/pool_allocator.hpp"

namespace openperf::packetio::dpdk::mempool {

static std::atomic_uint mempool_idx = 0;

template <typename T, typename S>
__attribute__((const)) static T align_up(T x, S align)
{
    return ((x + align - 1) & ~(align - 1));
}

/*
 * Based on the DPDK rte_pktmbuf_pool_create() function, but optimized for
 * single-producer, single-consumer use. Since we create pools on a per
 * port-queue basis, each memory pool will only be used in a single thread.
 */
struct rte_mempool* create_spsc_pktmbuf_mempool(std::string_view id,
                                                unsigned int n,
                                                unsigned int cache_size,
                                                uint16_t priv_size,
                                                uint16_t packet_size,
                                                unsigned socket_id)
{
    if (RTE_ALIGN(priv_size, RTE_MBUF_PRIV_ALIGN) != priv_size) {
        OP_LOG(OP_LOG_ERROR, "mbuf priv_size=%u is not aligned\n", priv_size);
        rte_errno = EINVAL;
        return (nullptr);
    }

    auto max_packet_size = align_up(packet_size, 64);
    auto elt_size = sizeof(rte_mbuf) + priv_size + max_packet_size;

    auto mp = rte_mempool_create(id.data(),
                                 n,
                                 elt_size,
                                 cache_size,
                                 sizeof(rte_pktmbuf_pool_private),
                                 rte_pktmbuf_pool_init,
                                 nullptr,
                                 rte_pktmbuf_init,
                                 nullptr,
                                 static_cast<int>(socket_id),
                                 MEMPOOL_F_SP_PUT | MEMPOOL_F_SC_GET);

    return (mp);
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

rte_mempool* acquire(unsigned numa_node,
                     uint16_t packet_length,
                     uint16_t packet_count,
                     uint16_t cache_size)
{
    /* packet_count must be a Mersenne prime */
    assert(!((packet_count + 1) & packet_count));

    auto name =
        "pool-"
        + std::to_string(mempool_idx.fetch_add(1, std::memory_order_acq_rel));

    auto* pool = create_spsc_pktmbuf_mempool(
        name.c_str(), packet_count, cache_size, 0, packet_length, numa_node);
    if (!pool) {
        OP_LOG(OP_LOG_ERROR, "Could not create packet pool %s\n", name.c_str());
        return (nullptr);
    }

    log_mempool(pool);

    return (pool);
}

void release(const rte_mempool* pool)
{
    if (!pool) { return; }

    OP_LOG(OP_LOG_DEBUG, "Freeing memory pool %s\n", pool->name);
    rte_mempool_free(const_cast<rte_mempool*>(pool));
}

rte_mempool* get_default(unsigned numa_node)
{
    return (primary::pool_allocator::instance().get_mempool(numa_node));
}

} // namespace openperf::packetio::dpdk::mempool
