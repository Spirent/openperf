#include "config/op_config_file.hpp"
#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/mempool.hpp"
#include "packetio/memory/dpdk/secondary/memory_priv.h"

namespace openperf::packetio::dpdk::mempool {

static rte_mempool* get_mempool()
{
    auto name = config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
        op_packetio_dpdk_mempool);
    if (!name) {
        OP_LOG(OP_LOG_WARNING, "No DPDK mbuf memory pool has been specified\n");
        return (nullptr);
    }

    auto pool = rte_mempool_lookup(name.value().c_str());
    if (!pool) {
        OP_LOG(OP_LOG_ERROR,
               "Could not find memory pool named %s\n",
               name.value().c_str());
        return (nullptr);
    }

    return (pool);
}

rte_mempool* acquire([[maybe_unused]] uint16_t port_id,
                     [[maybe_unused]] uint16_t queue_id,
                     unsigned numa_node,
                     uint16_t packet_length,
                     uint16_t packet_count,
                     [[maybe_unused]] uint16_t cache_size,
                     [[maybe_unused]] mempool_type pool_type)
{
    static rte_mempool* pool = get_mempool();
    if (!pool) { throw std::runtime_error("No memory pool available"); }

    if (pool->elt_size < packet_length) {
        OP_LOG(OP_LOG_WARNING,
               "Memory pool %s elements are too small: "
               "requested = %u, available = %u\n",
               pool->name,
               packet_length,
               pool->elt_size);
    }
    if (pool->size < packet_count) {
        OP_LOG(OP_LOG_WARNING,
               "Memory pool %s size is too small: "
               "requested = %u, max available = %u\n",
               pool->name,
               packet_count,
               pool->size);
    }
    if (pool->socket_id != static_cast<int>(numa_node)) {
        OP_LOG(OP_LOG_WARNING,
               "Memory pool is in a different NUMA zone than requested: "
               "requested = %u, actual = %u\n",
               numa_node,
               pool->socket_id);
    }

    return (pool);
}

void release(const rte_mempool*)
{
    /* No-op since secondary processes can't create mempools */
}

rte_mempool* get_default(unsigned) { return (get_mempool()); }

} // namespace openperf::packetio::dpdk::mempool
