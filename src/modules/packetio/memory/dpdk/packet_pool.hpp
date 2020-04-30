#ifndef _OP_PACKETIO_MEMORY_DPDK_PACKET_POOL_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_PACKET_POOL_HPP_

#include <cassert>
#include <memory>
#include <string>

#include "packetio/drivers/dpdk/dpdk.h"

struct rte_mempool;

namespace openperf::packetio {
namespace packet {
struct packet_buffer;
}

namespace dpdk {

class packet_pool
{
    std::shared_ptr<rte_mempool> m_pool;

public:
    packet_pool(std::string_view id,
                unsigned numa_node,
                uint16_t packet_length,
                uint16_t packet_count,
                uint16_t cache_size = 0);
    ~packet_pool() = default;

    packet::packet_buffer* get();
    uint16_t get(packet::packet_buffer* buffers[], uint16_t count);

    void put(packet::packet_buffer* buffer);
    void put(packet::packet_buffer* const buffers[], uint16_t count);
};

} // namespace dpdk
} // namespace openperf::packetio

#endif /* _OP_PACKETIO_MEMORY_DPDK_PACKET_POOL_HPP_ */
