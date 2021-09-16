#ifndef _OP_PACKETIO_DPDK_PORT_STATS_HPP_
#define _OP_PACKETIO_DPDK_PORT_STATS_HPP_

#include <cstdint>

namespace openperf::packetio::dpdk::port_stats {

uint64_t tx_deferred(uint16_t port_id);

void tx_deferred_add(uint16_t port_id, uint64_t inc);

} // namespace openperf::packetio::dpdk::port_stats

#endif /* _OP_PACKETIO_DPDK_PORT_STATS_HPP_ */
