#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/memory.h"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/memory/dpdk/primary/mempool_fmt.h"

const char memp_default_mempool_fmt[] = "DEFAULT_%d";
const char memp_ref_rom_mempool_fmt[] = "REF_ROM_%d";

typedef _Atomic(struct rte_mempool*) atomic_mempool_ptr;

static atomic_mempool_ptr memp_default_mempools[RTE_MAX_NUMA_NODES] = {};
static atomic_mempool_ptr memp_ref_rom_mempools[RTE_MAX_NUMA_NODES] = {};

static struct rte_mempool* get_mempool_by_id(const char* memp_fmt, unsigned id)
{
    char name_buf[RTE_MEMPOOL_NAMESIZE];
    snprintf(name_buf, RTE_MEMPOOL_NAMESIZE, memp_fmt, id);
    return (rte_mempool_lookup(name_buf));
}

/* Retrieve the first mempool we can find */
static struct rte_mempool* get_mempool(const char* memp_fmt)
{
    for (size_t i = 0; i < RTE_MAX_NUMA_NODES; i++) {
        struct rte_mempool* mpool = get_mempool_by_id(memp_fmt, i);
        if (mpool) return (mpool);
    }

    return (NULL);
}

/* Populate the mempool array with a pool for each index. */
static void initialize_mempools(const char* memp_fmt,
                                atomic_mempool_ptr mpools[],
                                size_t length)
{
    struct rte_mempool* default_mpool = get_mempool(memp_fmt);
    assert(default_mpool);

    for (size_t i = 0; i < length; i++) {
        struct rte_mempool* mpool = get_mempool_by_id(memp_fmt, i);
        atomic_store_explicit(&mpools[i],
                              (mpool == NULL ? default_mpool : mpool),
                              memory_order_relaxed);
    }
}

static struct rte_mempool* load_mempool(atomic_mempool_ptr mpools[],
                                        unsigned idx)
{
    return (atomic_load_explicit(&mpools[idx], memory_order_relaxed));
}

void packetio_memory_initialize()
{
    /*
     * Make sure that our mempool arrays are populated with a mempool for
     * every socket.
     */
    initialize_mempools(
        memp_default_mempool_fmt, memp_default_mempools, RTE_MAX_NUMA_NODES);
    initialize_mempools(
        memp_ref_rom_mempool_fmt, memp_ref_rom_mempools, RTE_MAX_NUMA_NODES);
}

struct pbuf* packetio_memory_pbuf_default_alloc()
{
    return (packetio_memory_mbuf_to_pbuf(rte_pktmbuf_alloc(
        load_mempool(memp_default_mempools, rte_socket_id()))));
}

struct pbuf* packetio_memory_pbuf_ref_rom_alloc()
{
    return (packetio_memory_mbuf_to_pbuf(rte_pktmbuf_alloc(
        load_mempool(memp_ref_rom_mempools, rte_socket_id()))));
}

void packetio_memory_pbuf_free(struct pbuf* p)
{
    rte_pktmbuf_free_seg(packetio_memory_pbuf_to_mbuf(p));
}

void* packetio_memory_alloc(const char* desc, size_t size)
{
    return (rte_malloc_socket(desc, size, 0, rte_socket_id()));
}

void packetio_memory_free(void* ptr) { rte_free(ptr); }

int64_t packetio_memory_memp_pool_avail(const struct memp_desc* mem)
{
    int64_t total_avail = 0;
    if (strncmp(mem->desc, "PBUF_POOL", 9) == 0) {
        for (int i = 0; i < RTE_MAX_NUMA_NODES; i++) {
            struct rte_mempool* node_mpool =
                get_mempool_by_id(memp_default_mempool_fmt, i);
            if (node_mpool) {
                total_avail += rte_mempool_avail_count(node_mpool);
            }
        }
    } else if (strncmp(mem->desc, "PBUF_REF/ROM", 12) == 0) {
        for (int i = 0; i < RTE_MAX_NUMA_NODES; i++) {
            struct rte_mempool* node_mpool =
                get_mempool_by_id(memp_ref_rom_mempool_fmt, i);
            if (node_mpool) {
                total_avail += rte_mempool_avail_count(node_mpool);
            }
        }
    } else {
        total_avail = mem->stats->avail;
    }

    return (total_avail);
}

int64_t packetio_memory_memp_pool_max(const struct memp_desc* mem)
{
    int64_t max = 0;
    if (strncmp(mem->desc, "PBUF_POOL", 9) == 0
        || strncmp(mem->desc, "PBUF_REF/ROM", 12) == 0) {
        max = -1;
    } else {
        max = mem->stats->max;
    }

    return (max);
}

int64_t packetio_memory_memp_pool_used(const struct memp_desc* mem)
{
    int64_t total_used = 0;
    if (strncmp(mem->desc, "PBUF_POOL", 9) == 0) {
        for (int i = 0; i < RTE_MAX_NUMA_NODES; i++) {
            struct rte_mempool* node_mpool =
                get_mempool_by_id(memp_default_mempool_fmt, i);
            if (node_mpool) {
                total_used += rte_mempool_in_use_count(node_mpool);
            }
        }
    } else if (strncmp(mem->desc, "PBUF_REF/ROM", 12) == 0) {
        for (int i = 0; i < RTE_MAX_NUMA_NODES; i++) {
            struct rte_mempool* node_mpool =
                get_mempool_by_id(memp_ref_rom_mempool_fmt, i);
            if (node_mpool) {
                total_used += rte_mempool_in_use_count(node_mpool);
            }
        }
    } else {
        total_used = mem->stats->used;
    }

    return (total_used);
}
