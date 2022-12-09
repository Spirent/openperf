#ifndef _OP_PACKETIO_DPDK_WORKER_GRO_HPP_
#define _OP_PACKETIO_DPDK_WORKER_GRO_HPP_

#include <cstdint>

struct rte_mbuf;

namespace openperf::packetio::dpdk::worker {

uint16_t do_software_gro(struct rte_mbuf* packets[], uint16_t nb_packets);

} // namespace openperf::packetio::dpdk::worker

#endif /* _OP_PACKETIO_DPDK_WORKER_GRO_HPP_ */
