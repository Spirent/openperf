#ifndef _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_
#define _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_

#include <cstdint>

struct rte_mbuf;

namespace openperf {
namespace packetio {
namespace dpdk {

void set_tx_offload_metadata(rte_mbuf* mbuf, uint16_t mtu);

}
}
}

#endif /* _OP_PACKETIO_STACK_DPDK_OFFLOAD_UTILS_H_ */
