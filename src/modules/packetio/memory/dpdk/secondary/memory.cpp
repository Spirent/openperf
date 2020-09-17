#include <cassert>

#include "config/op_config_file.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/memory.h"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/memory/dpdk/secondary/memory_priv.h"

static std::atomic<rte_mempool*> op_packetio_dpdk_memory_pool = nullptr;

static rte_mempool* get_mempool()
{
    auto name =
        openperf::config::file::op_config_get_param<OP_OPTION_TYPE_STRING>(
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

    if (pool->private_data_size <= sizeof(pbuf)) {
        OP_LOG(OP_LOG_ERROR,
               "Memory pool %s private size (%u) is less than "
               "required minimum size of %zu\n",
               pool->name,
               pool->private_data_size,
               sizeof(pbuf));
        return (nullptr);
    }
    return (pool);
}

extern "C" {

void packetio_memory_initialize()
{
    auto* pool = get_mempool();
    assert(pool);
    op_packetio_dpdk_memory_pool.store(pool, std::memory_order_release);
}

void* packetio_memory_alloc(const char* desc, size_t size)
{
    return (rte_malloc_socket(desc, size, 0, rte_socket_id()));
}

void packetio_memory_free(void* ptr) { rte_free(ptr); }

struct pbuf* packetio_memory_pbuf_default_alloc()
{
    return (packetio_memory_mbuf_to_pbuf(rte_pktmbuf_alloc(
        op_packetio_dpdk_memory_pool.load(std::memory_order_acquire))));
}

struct pbuf* packetio_memory_pbuf_ref_rom_alloc()
{
    return (packetio_memory_pbuf_default_alloc());
}

void packetio_memory_pbuf_free(struct pbuf* p)
{
    rte_pktmbuf_free(packetio_memory_pbuf_to_mbuf(p));
}

int64_t packetio_memory_memp_pool_avail(const struct memp_desc*) { return (0); }

int64_t packetio_memory_memp_pool_max(const struct memp_desc*) { return (0); }

int64_t packetio_memory_memp_pool_used(const struct memp_desc*) { return (0); }
}
