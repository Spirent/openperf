#ifndef _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_

#include <map>
#include <memory>

#include "core/op_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/queue_utils.hpp"
#include "utils/singleton.hpp"

namespace openperf::packetio::dpdk::primary {

class pool_allocator : public utils::singleton<pool_allocator>
{
public:
    void init(const std::vector<uint16_t>&,
              const std::map<uint16_t, queue::count>&);
    void fini();

    rte_mempool* get_mempool(unsigned socket_id) const;

private:
    struct rte_mempool_releaser
    {
        void operator()(rte_mempool* mp)
        {
            if (!rte_mempool_full(mp)) {
                OP_LOG(OP_LOG_ERROR,
                       "mempool %s is missing %u mbufs\n",
                       mp->name,
                       rte_mempool_in_use_count(mp));
            }
            rte_mempool_free(mp);
        }
    };

    typedef std::unique_ptr<rte_mempool, rte_mempool_releaser> mempool_ptr;
    std::map<unsigned, mempool_ptr> m_pools;

    static constexpr std::string_view mempool_format = "pool_alloc_%u";
};

} // namespace openperf::packetio::dpdk::primary

#endif /* _OP_PACKETIO_MEMORY_DPDK_PRIMARY_POOL_ALLOCATOR_HPP_ */
