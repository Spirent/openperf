#ifndef _PACKETIO_DRIVER_DPDK_H_
#define _PACKETIO_DRIVER_DPDK_H_

/**
 * A single file to pull in all of the 'standard' DPDK headers
 */

#include <sys/types.h>
#include <linux/limits.h>

#include "rte_config.h"

/* XXX: Fixup defines when using gcc/DPDK builds with llvm */
#if defined(__clang__) && !defined(RTE_TOOLCHAIN_CLANG)
#undef RTE_TOOLCHAIN_GCC
#define RTE_TOOLCHAIN_CLANG
#endif

#include "rte_common.h"
#include "rte_byteorder.h"
#include "rte_log.h"
#include "rte_debug.h"
#include "rte_cycles.h"
#include "rte_memory.h"
#include "rte_memcpy.h"
#include "rte_memzone.h"
#include "rte_launch.h"
#include "rte_eal.h"
#include "rte_per_lcore.h"
#include "rte_lcore.h"
#include "rte_atomic.h"
#include "rte_branch_prediction.h"
#include "rte_ring.h"
#include "rte_mempool.h"
#include "rte_malloc.h"
#include "rte_mbuf.h"
#include "rte_mbuf_pool_ops.h"
#include "rte_interrupts.h"
#include "rte_pci.h"
#include "rte_ether.h"
#include "rte_ethdev.h"
#include "rte_eth_bond.h"
#include "rte_dev.h"
#include "rte_string_fns.h"
#include "rte_errno.h"
#include "rte_version.h"
#include "rte_ether.h"
#include "rte_gro.h"
#include "rte_ring.h"
#include "rte_eth_ring.h"
#include "rte_flow_driver.h"
#include "rte_net.h"
#include "rte_thash.h"

#ifdef __cplusplus
namespace openperf::packetio::dpdk {
static constexpr auto mbuf_prefetch_offset = 8;
}
#endif

#endif /* _PACKETIO_DRIVER_DPDK_H_ */
