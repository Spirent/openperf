#ifndef _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_HPP_

#include <cstdint>
#include <string>

struct rte_mempool;

namespace openperf::packetio::dpdk {

rte_mempool* mempool_acquire(std::string_view id,
                             unsigned numa_node,
                             uint16_t packet_length,
                             uint16_t packet_count,
                             uint16_t cache_size);

void mempool_release(const rte_mempool*);

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_HPP_ */
