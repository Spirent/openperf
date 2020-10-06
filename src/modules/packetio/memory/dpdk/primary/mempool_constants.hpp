#ifndef _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_CONSTANTS_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_CONSTANTS_HPP_

#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::dpdk {

/*
 * Provide additional space between the mbuf header and payload for stack usage.
 * Adjust as necessary.
 */
inline constexpr auto mempool_private_size = 64;

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_MEMORY_DPDK_MEMPOOL_CONSTANTS_HPP_ */
