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
    for (auto i = 0; i < RTE_MAX_NUMA_NODES; ++i) {
        memp_mempools[i] = openperf::packetio::dpdk::mempool::get_default(i);
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
    return (packet_stack_mbuf_to_pbuf(
        rte_pktmbuf_alloc(memp_mempools[rte_socket_id()])));
}

void packet_stack_pbuf_free(struct pbuf* p)
{
    rte_pktmbuf_free_seg(packet_stack_pbuf_to_mbuf(p));
}
}
