#ifndef _OP_PACKETIO_DPDK_WORKER_TX_FUNCTIONS_HPP_
#define _OP_PACKETIO_DPDK_WORKER_TX_FUNCTIONS_HPP_

#include <cstdint>

struct rte_mbuf;

namespace openperf::packetio::dpdk::worker {

uint16_t tx_copy_direct(int idx, uint32_t hash, struct rte_mbuf* mbufs[],
                        uint16_t nb_mbufs);
uint16_t tx_copy_queued(int idx, uint32_t hash, struct rte_mbuf* mbufs[],
                        uint16_t nb_mbufs);

uint16_t tx_direct(int idx, uint32_t hash, struct rte_mbuf* mbufs[],
                   uint16_t nb_mbufs);
uint16_t tx_queued(int idx, uint32_t hash, struct rte_mbuf* mbufs[],
                   uint16_t nb_mbufs);

uint16_t tx_dummy(int idx, uint32_t hash, struct rte_mbuf* mbufs[],
                  uint16_t nb_mbufs);

} // namespace openperf::packetio::dpdk::worker

#endif /* _OP_PACKETIO_DPDK_WORKER_TX_FUNCTIONS_HPP_ */
