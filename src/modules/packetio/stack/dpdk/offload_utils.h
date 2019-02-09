#ifndef _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_
#define _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_

#include <cstdint>

struct rte_mbuf;

namespace icp {
namespace packetio {
namespace dpdk {

void set_tx_offload_metadata(rte_mbuf* mbuf, uint16_t mtu);

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_ */
