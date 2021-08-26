#ifndef _OP_PACKETIO_DRIVER_DPDK_QUIRKS_HPP_
#define _OP_PACKETIO_DRIVER_DPDK_QUIRKS_HPP_

#include <cstdint>

namespace openperf::packetio::dpdk::quirks {

/*
 * Provide a single location for enumerating problematic
 * driver behavior. While drivers have a common API, they
 * tend to have quirky behavior which requires adjustment.
 * The intent is to capture all of this quirkiness here.
 */

/*
 * Adjust configured packet lengths when configuring queues.
 * Drivers don't use the dataroom/headroom split in DPDK consistently.
 */
uint16_t adjust_max_rx_pktlen(uint16_t port_id);

/*
 * Override the reported max packet length. Some drivers outright lie
 * about the packet sizes they support while others report ridiculous
 * values.
 */
uint32_t sane_max_rx_pktlen(uint16_t port_id);

/*
 * Specify offloads that are technically supported by the driver
 * but are undesirable, e.g. an offload that disables vectorized
 * packet processing.
 */
uint64_t slow_rx_offloads(uint16_t port_id);
uint64_t slow_tx_offloads(uint16_t port_id);

} // namespace openperf::packetio::dpdk::quirks

#endif /* _OP_PACKETIO_DRIVER_DPDK_QUIRKS_HPP_ */
