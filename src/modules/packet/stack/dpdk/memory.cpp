#include <array>
#include <algorithm>
#include <string>

#include "lwip/memp.h"

#include "packet/stack/dpdk/pbuf_utils.h"
#include "packet/stack/lwip/memory.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/mempool.hpp"

extern "C" {

/* List of names used by LwIP's packet buffer memory pools */
constexpr auto lwip_pbuf_pool_names =
    std::array<std::string_view, 2>{"PBUF_POOL", "PBUF_REF/ROM"};

static rte_mempool* memp_mempools[RTE_MAX_NUMA_NODES] = {};

void packet_stack_memp_initialize()
{
    struct rte_mempool* default_mempool = nullptr;

    for (auto i = 0; i < RTE_MAX_NUMA_NODES; ++i) {
        memp_mempools[i] = openperf::packetio::dpdk::mempool::get_default(i);
        if (!default_mempool && memp_mempools[i]) {
            default_mempool = memp_mempools[i];
        }
    }

    // Verify default memory pool was found
    if (!default_mempool) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to locate any mempools for the TCP/IP stack");
        return;
    }

    // Ideally the NUMA node for the CPUs and devices will be the same, however
    // due to misconfiguration or system resource limitations, this may not be
    // true.  Therefore need to verify that there is a valid mempool for all
    // lcore workers. If there is no valid mempool, then use the default
    // mempool.
    unsigned int lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id)
    {
        auto socket_id = rte_lcore_to_socket_id(lcore_id);
        if (socket_id == static_cast<unsigned>(SOCKET_ID_ANY)) {
            socket_id = 0;
            OP_LOG(OP_LOG_INFO,
                   "lcore %u is not bound to a NUMA socket.  "
                   "Using NUMA %u mempool by default for the TCP/IP stack.",
                   lcore_id,
                   socket_id);
        }
        if (!memp_mempools[socket_id]) {
            OP_LOG(OP_LOG_WARNING,
                   "No NUMA mempool for NUMA %u.  "
                   "Using mempool %s instead for the TCP/IP stack.",
                   socket_id,
                   default_mempool->name);
            memp_mempools[socket_id] = default_mempool;
        }
    }
}

void* packet_stack_memp_alloc(const char* desc, size_t size)
{
    static bool is_primary = rte_eal_process_type() == RTE_PROC_PRIMARY;
    return (is_primary ? rte_malloc(desc, size, 0) : malloc(size));
}

void packet_stack_memp_free(void* ptr)
{
    static bool is_primary = rte_eal_process_type() == RTE_PROC_PRIMARY;
    return (is_primary ? rte_free(ptr) : free(ptr));
}

int64_t packet_stack_memp_pool_avail(const struct memp_desc* mem)
{
    using namespace openperf::packetio::dpdk;

    int64_t total_avail = 0;

    auto cursor = std::find(std::begin(lwip_pbuf_pool_names),
                            std::end(lwip_pbuf_pool_names),
                            mem->desc);
    if (cursor != std::end(lwip_pbuf_pool_names)) {
        for (auto i = 0; i < RTE_MAX_NUMA_NODES; ++i) {
            if (auto* pool = mempool::get_default(i)) {
                total_avail +=
                    pool->socket_id == i ? rte_mempool_avail_count(pool) : 0;
            }
        }
    } else {
        total_avail = mem->stats->avail;
    }

    return (total_avail);
}

int64_t packet_stack_memp_pool_max(const struct memp_desc* mem)
{
    auto cursor = std::find(std::begin(lwip_pbuf_pool_names),
                            std::end(lwip_pbuf_pool_names),
                            mem->desc);
    return (cursor == std::end(lwip_pbuf_pool_names) ? mem->stats->max : -1);
}

int64_t packet_stack_memp_pool_used(const struct memp_desc* mem)
{
    using namespace openperf::packetio::dpdk;

    int64_t total_used = 0;

    auto cursor = std::find(std::begin(lwip_pbuf_pool_names),
                            std::end(lwip_pbuf_pool_names),
                            mem->desc);
    if (cursor != std::end(lwip_pbuf_pool_names)) {
        for (auto i = 0; i < RTE_MAX_NUMA_NODES; ++i) {
            if (auto* pool = mempool::get_default(i)) {
                total_used +=
                    pool->socket_id == i ? rte_mempool_in_use_count(pool) : 0;
            }
        }
    } else {
        total_used = mem->stats->used;
    }

    return (total_used);
}

struct pbuf* packet_stack_pbuf_alloc()
{
    auto socket_id = rte_socket_id();
    if (socket_id == static_cast<unsigned>(SOCKET_ID_ANY)) {
        // Default to NUMA node 0 when socket ID is not set
        // Socket ID is not set when lcore cpu set spans NUMA nodes
        socket_id = 0;
    }
    return (
        packet_stack_mbuf_to_pbuf(rte_pktmbuf_alloc(memp_mempools[socket_id])));
}

void packet_stack_pbuf_free(struct pbuf* p)
{
    rte_pktmbuf_free_seg(packet_stack_pbuf_to_mbuf(p));
}
}
