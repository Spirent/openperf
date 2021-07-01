#ifndef _OP_PACKETIO_DPDK_TX_MEMPOOL_ALLOCATOR_HPP_
#define _OP_PACKETIO_DPDK_TX_MEMPOOL_ALLOCATOR_HPP_

#include <map>
#include <set>

#include "packetio/memory/dpdk/mempool.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"

struct rte_mempool;

namespace openperf::packetio::dpdk {

class tx_mempool_allocator
{
    struct mempool_entry
    {
        struct rte_mempool* pool;
        mutable unsigned ref_count;
        uint16_t port_id;
        uint16_t queue_id;
    };

    friend bool operator<(const mempool_entry& lhs, const mempool_entry& rhs);

    std::set<mempool_entry> m_pools; /* set of all shared pools */
    std::map<std::string, struct rte_mempool*, std::less<>>
        m_source_pools; /* map from source id -> pool */
    worker::recycler& m_recycler;

public:
    tx_mempool_allocator(worker::recycler& recycler);

    struct rte_mempool* acquire(uint16_t port_id,
                                uint16_t queue_id,
                                const packet::generic_source& source);

    void release(std::string_view id);
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_TX_MEMPOOL_ALLOCATOR_HPP_ */
