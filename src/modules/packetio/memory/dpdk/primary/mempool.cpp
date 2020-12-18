#include <mutex>

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/mempool.hpp"
#include "packetio/memory/dpdk/primary/pool_allocator.hpp"

namespace openperf::packetio::dpdk::mempool {

struct mempool_entry
{
    rte_mempool* pool;
    op_reference ref;
    uint16_t port_id;
    uint16_t queue_id;
};

/*
 * Note: This is a lock-free list and is safe for multiple threads
 * to use.
 */
using mempool_list_type = openperf::list<mempool_entry>;
static mempool_list_type mempool_list;
static std::atomic_uint mempool_idx = 0;

/*
 * Note: container_of is defined in DPDK's rte_common.h file. Unfortunately,
 * clang-tidy hates that macro, so we create a simpler, saner one here.
 */
#define local_container_of(ptr, type, member)                                  \
    ((type*)((char*)(ptr)-offsetof(type, member)))

static void drop_mempool_entry(const struct op_reference* ref)
{
    auto* entry = local_container_of(ref, mempool_entry, ref);
    mempool_list.remove(entry);
    rte_mempool_free(entry->pool);
    entry->pool = nullptr;
}

static auto tie_entry(const mempool_entry* entry)
{
    /* XXX: the list implementation makes some questionable comparisons */
    if (!entry || !entry->pool) {
        const auto zero16 = uint16_t{0};
        auto zero32 = uint32_t{0}; /* intentionally not const */
        return (std::tie(zero16, zero16, zero32));
    }

    return (std::tie(entry->port_id, entry->queue_id, entry->pool->elt_size));
}

class mempool_entry_guard
{
    mempool_entry& m_entry;

public:
    mempool_entry_guard(mempool_entry& entry)
        : m_entry(entry)
    {
        op_reference_retain(&m_entry.ref);
    }

    ~mempool_entry_guard() { op_reference_release(&m_entry.ref); }
};

static int mempool_entry_comparator(const void* thing1, const void* thing2)
{
    assert(thing1);
    assert(thing2);

    const auto lhs = tie_entry(static_cast<const mempool_entry*>(thing1));
    const auto rhs = tie_entry(static_cast<const mempool_entry*>(thing2));

    if (lhs < rhs) { return (-1); }
    if (lhs > rhs) { return (1); }
    return (0);
}

static void mempool_entry_destructor(void* thing)
{
    auto* entry = static_cast<mempool_entry*>(thing);
    delete entry;
}

/*
 * We want to sort our list by port, queue, and packet size respectively.
 * That way, acquire calls will always find the memory pool with the smallest
 * packet size that works with the request.
 */
static void maybe_mempool_list_init()
{
    static std::once_flag mempool_list_init_flag;
    std::call_once(mempool_list_init_flag, [&]() {
        mempool_list.set_comparator(mempool_entry_comparator);
        mempool_list.set_destructor(mempool_entry_destructor);
    });
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

/*
 * XXX: We are forced to use new/delete pairs for our entry objects
 * because the c-based list implementation only accepts pointers.
 */
rte_mempool* acquire(uint16_t port_id,
                     uint16_t queue_id,
                     unsigned numa_node,
                     uint16_t packet_length,
                     uint16_t packet_count,
                     uint16_t cache_size)
{
    maybe_mempool_list_init();

    /* See if we can find a matching pool in our list of pools */
    auto snapshot = mempool_list.snapshot();
    auto cursor = std::find_if(
        std::begin(snapshot), std::end(snapshot), [&](auto& entry) {
            auto guard = mempool_entry_guard(entry);

            auto match =
                (entry.pool != nullptr && port_id == entry.port_id
                 && queue_id == entry.queue_id
                 && numa_node == static_cast<unsigned>(entry.pool->socket_id)
                 && packet_length <= entry.pool->elt_size
                 && packet_count <= entry.pool->size
                 && cache_size <= entry.pool->cache_size);

            if (match) {
                /* We want to keep this mempool; bump the reference count */
                op_reference_retain(&entry.ref);
            }

            return (match);
        });

    if (cursor != std::end(snapshot)) {
        assert(cursor->pool);

        OP_LOG(OP_LOG_DEBUG,
               "Reusing packet pool %s (refcount = %u)\n",
               cursor->pool->name,
               atomic_load_explicit(&(cursor->ref.count),
                                    std::memory_order_relaxed));
        return (cursor->pool);
    }

    /* No matching mempool found; create one */
    auto name =
        "pool-"
        + std::to_string(mempool_idx.fetch_add(1, std::memory_order_acq_rel));

    auto* pool = create_spsc_pktmbuf_mempool(name.c_str(),
                                             pool_size_adjust(packet_count),
                                             cache_size,
                                             0,
                                             packet_length,
                                             numa_node);
    if (!pool) {
        OP_LOG(OP_LOG_ERROR, "Could not create packet pool %s\n", name.c_str());
        return (nullptr);
    }

    log_mempool(pool);

    auto* entry = new mempool_entry{
        .pool = pool, .port_id = port_id, .queue_id = queue_id};
    if (!entry) { throw std::bad_alloc(); }

    op_reference_init(&entry->ref, drop_mempool_entry);

    if (!mempool_list.insert(entry)) {
        OP_LOG(OP_LOG_ERROR,
               "Could not insert packet pool %s into packet pool list\n",
               name.c_str());
        op_reference_release(&entry->ref);
        return (nullptr);
    }

    return (entry->pool);
}

void release(const rte_mempool* pool)
{
    if (!pool) { return; }

    auto snapshot = mempool_list.snapshot();
    auto cursor = std::find_if(
        std::begin(snapshot), std::end(snapshot), [&](auto& entry) {
            auto guard = mempool_entry_guard(entry);
            /*
             * If the pool matches the one we are releasing, then
             * the ref count can't possibly drop to 0 when we
             * drop our guard.
             */
            return (entry.pool == pool);
        });

    if (cursor == std::end(snapshot)) {
        OP_LOG(OP_LOG_WARNING,
               "Could not find %s in packet pool list; "
               "ignoring release request\n",
               pool->name);
        return;
    }

    op_reference_release(&(cursor->ref));
}

rte_mempool* get_default(unsigned numa_node)
{
    return (primary::pool_allocator::instance().get_mempool(numa_node));
}

} // namespace openperf::packetio::dpdk::mempool
