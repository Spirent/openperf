#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/workers/dpdk/tx_mempool_allocator.hpp"

namespace openperf::packetio::dpdk {

/*
 * Convert nb_mbufs to a Mersenne number, as those are the
 * most efficient size for mempools.  If our input is already a
 * power of 2, return input - 1 instead of doubling the size.
 */
static unsigned pool_size_adjust(unsigned nb_mbufs)
{
    return (rte_is_power_of_2(nb_mbufs) ? nb_mbufs - 1
                                        : rte_align32pow2(nb_mbufs) - 1);
}

static unsigned packet_count(uint16_t port_id)
{
    return (pool_size_adjust(port_info::tx_desc_count(port_id)
                             + 2 * worker::pkt_burst_size));
}

/*
 * Order pool entries by port/queue/packet size. When we
 * search for matching pools, we should find the one with
 * the lowest packet size that works.
 */
bool operator<(const tx_mempool_allocator::mempool_entry& lhs,
               const tx_mempool_allocator::mempool_entry& rhs)
{
    return (std::tie(lhs.port_id, lhs.queue_id, lhs.pool->elt_size)
            < std::tie(rhs.port_id, rhs.queue_id, rhs.pool->elt_size));
}

tx_mempool_allocator::tx_mempool_allocator(worker::recycler& recycler)
    : m_recycler(recycler)
{}

static struct rte_mempool* get_mempool(uint16_t port_id,
                                       uint16_t max_packet_length)
{
    /*
     * A note about pool and cache sizing: we want to make sure we have
     * packets available if the NIC's transmit ring is completely full, so we
     * need our pool size to be a little larger than the ring size.  Similarly,
     * we need the cache to be larger than the worker's standard transaction
     * size, otherwise every call to retrieve buffers from the pool will bypass
     * the CPU cache and go straight to the pool.
     */
    return (mempool::acquire(port_info::socket_id(port_id),
                             max_packet_length + RTE_PKTMBUF_HEADROOM,
                             packet_count(port_id),
                             2 * worker::pkt_burst_size));
}

struct rte_mempool* tx_mempool_allocator::acquire(
    uint16_t port_id, uint16_t queue_id, const packet::generic_source& source)
{
    bool uses_signature = source.uses_feature(
        packet::source_feature_flags::spirent_signature_encode);
    if (!uses_signature) {
        /* Non-signature sources need their own pools */
        auto* pool = get_mempool(port_id, source.max_packet_length());
        m_source_pools.emplace(source.id(), pool);
        return (pool);
    }

    /* See if we can reuse an existing pool */
    auto cursor =
        std::find_if(std::begin(m_pools), std::end(m_pools), [&](auto&& entry) {
            auto match =
                (port_id == entry.port_id && queue_id == entry.queue_id
                 && source.max_packet_length() <= entry.pool->elt_size);

            return (match);
        });

    /* Looks like we can; bump the ref count and return it */
    if (cursor != std::end(m_pools)) {
        assert(cursor->pool);
        m_source_pools.emplace(source.id(), cursor->pool);
        cursor->ref_count++;
        OP_LOG(OP_LOG_DEBUG,
               "Reusing tx packet pool %s for source %s (refcount = %u)\n",
               cursor->pool->name,
               source.id().c_str(),
               cursor->ref_count);
        return (cursor->pool);
    }

    /* No matching mempool found; allocate a new one */
    auto* pool = get_mempool(port_id, source.max_packet_length());
    if (!pool) { return (nullptr); }

    m_source_pools.emplace(source.id(), pool);
    m_pools.emplace(mempool_entry{pool, 1, port_id, queue_id});

    OP_LOG(OP_LOG_DEBUG,
           "Allocated fresh tx packet pool %s for source %s\n",
           pool->name,
           source.id().c_str());
    return (pool);
}

static worker::recycler::gc_callback_result
do_mempool_release(const struct rte_mempool* pool)
{
    /*
     * We can't actually free the mempool until all of the
     * drivers have released any buffers they're holding onto.
     * So if the pool isn't full, we'll have to try again later.
     */
    if (!rte_mempool_full(pool)) {
        return (worker::recycler::gc_callback_result::retry);
    }

    mempool::release(pool);
    return (worker::recycler::gc_callback_result::ok);
}

void tx_mempool_allocator::release(std::string_view id)
{
    auto item = m_source_pools.find(id);
    if (item == std::end(m_source_pools)) {
        OP_LOG(OP_LOG_WARNING,
               "No tx packet pool found for source %.*s\n",
               static_cast<int>(id.size()),
               id.data());
        return;
    }

    auto* pool = item->second;
    m_source_pools.erase(item);

    OP_LOG(OP_LOG_DEBUG,
           "Releasing tx packet pool %s from source %.*s\n",
           pool->name,
           static_cast<int>(id.size()),
           id.data());

    /*
     * At this point, we don't know if this is a shared pool or not.
     * Look for it in the shared pool list.
     */
    auto cursor =
        std::find_if(std::begin(m_pools), std::end(m_pools), [&](auto&& entry) {
            return (entry.pool == pool);
        });

    if (cursor != std::end(m_pools)) {
        /* Definitely shared; drop the ref count and free if necessary */
        assert(cursor->ref_count > 0);
        if (--cursor->ref_count > 0) { return; }
        m_pools.erase(cursor);
    }

    /*
     * If we get here, the pool was either unique or needs to be freed;
     * do so.
     */
    OP_LOG(OP_LOG_DEBUG, "Recycling tx packet pool %s\n", pool->name);
    m_recycler.writer_add_gc_callback(
        [pool]() { return (do_mempool_release(pool)); });
}

} // namespace openperf::packetio::dpdk
