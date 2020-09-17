#ifndef _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_

#include <memory>
#include <unordered_map>

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/queue_utils.hpp"

namespace openperf::packetio::dpdk::primary {

class pool_allocator
{
public:
    pool_allocator(const std::vector<uint16_t>&,
                   const std::map<uint16_t, queue::count>&);

    rte_mempool* rx_mempool(unsigned socket_id) const;

private:
    struct rte_mempool_releaser
    {
        void operator()(rte_mempool* mp)
        {
            if (!rte_mempool_full(mp)) {
                OP_LOG(OP_LOG_ERROR,
                       "%mempool %s is missing %u mbufs\n",
                       mp->name,
                       rte_mempool_in_use_count(mp));
            }
            rte_mempool_free(mp);
        }
    };

    typedef std::unique_ptr<rte_mempool, rte_mempool_releaser> mempool_ptr;
    std::unordered_map<unsigned, mempool_ptr> m_default_mpools;
    std::unordered_map<unsigned, mempool_ptr> m_ref_rom_mpools;
};

} // namespace openperf::packetio::dpdk::primary

#endif /* _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_ */
