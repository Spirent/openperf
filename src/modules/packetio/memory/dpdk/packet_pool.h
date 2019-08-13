#ifndef _ICP_PACKETIO_MEMORY_DPDK_PACKET_POOL_H_
#define _ICP_PACKETIO_MEMORY_DPDK_PACKET_POOL_H_

#include <cassert>
#include <memory>
#include <string>

#include "packetio/drivers/dpdk/dpdk.h"

struct rte_mempool;

namespace icp::packetio {
namespace packets {
struct packet_buffer;
}

namespace dpdk {

class packet_pool
{
    std::shared_ptr<rte_mempool> m_pool;

public:
    packet_pool(std::string_view id, int numa_node,
                uint16_t packet_length, uint16_t packet_count,
                uint16_t cache_size = 0);
    ~packet_pool() = default;

    packets::packet_buffer* get();
    uint16_t get(packets::packet_buffer* buffers[], uint16_t count);

    void put(packets::packet_buffer* buffer);
    void put(packets::packet_buffer* const buffers[], uint16_t count);
};

}
}

#endif /* _ICP_PACKETIO_MEMORY_DPDK_PACKET_POOL_H_ */
