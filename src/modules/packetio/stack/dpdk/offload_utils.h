#ifndef _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_
#define _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_

struct rte_mbuf;

namespace icp {
namespace packetio {
namespace dpdk {

void set_tx_offload_metadata(rte_mbuf* mbuf);

}
}
}

#endif /* _ICP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_ */
