#ifndef _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_HPP_
#define _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_HPP_

#include <cstdint>

struct rte_mbuf;

namespace openperf::packet::stack::dpdk {

void set_tx_offload_metadata(rte_mbuf* mbuf, uint16_t mtu);

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_HPP_ */
