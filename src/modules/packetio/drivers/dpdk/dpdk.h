#ifndef _PACKETIO_DRIVER_DPDK_H_
#define _PACKETIO_DRIVER_DPDK_H_

/**
 * A single file to pull in all of the 'standard' DPDK headers
 */

#include <sys/types.h>
#include <linux/limits.h>

#include "rte_config.h"

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

#endif /* _PACKETIO_DRIVER_DPDK_H_ */
