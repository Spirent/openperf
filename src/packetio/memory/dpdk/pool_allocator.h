#ifndef _ICP_PACKETIO_MEMORY_DPDK_POOL_ALLOCATOR_H_
#define _ICP_PACKETIO_MEMORY_DPDK_POOL_ALLOCATOR_H_

#include <memory>
#include <vector>

#include "core/icp_core.h"
#include "drivers/dpdk/dpdk.h"
#include "drivers/dpdk/model/port_info.h"

namespace icp {
namespace packetio {
namespace dpdk {

class pool_allocator {
public:
    pool_allocator(const std::vector<model::port_info>&);

    rte_mempool * rx_mempool() { return default_pool.get(); }

private:
    struct rte_mempool_releaser {
        void operator()(rte_mempool *mp) {
            if (!rte_mempool_full(mp)) {
                ICP_LOG(ICP_LOG_ERROR, "%mempool %s is missing %u mbufs\n",
                        mp->name, rte_mempool_in_use_count(mp));
            }
            rte_mempool_free(mp);
        }
    };

    std::unique_ptr<rte_mempool, rte_mempool_releaser> default_pool;
    std::unique_ptr<rte_mempool, rte_mempool_releaser> ref_rom_pool;
};

}
}
}
#endif /* _ICP_PACKETIO_MEMORY_DPDK_POOL_ALLOCATOR_H_ */
