#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_

#include <chrono>

namespace openperf::packetio::dpdk::port::utils {

/*
 * Convenience type for Spirent signature timestamps, which consist of
 * 2.5 ns ticks.
 */
using phxtime = std::chrono::duration<int64_t, std::ratio<1, 400000000>>;

/*
 * Spirent signature fields use timestamps epochs that begin with
 * the current year.  This function calculates the proper offset
 * from the UNIX epoch.
 */
time_t get_timestamp_epoch_offset();

} // namespace openperf::packetio::dpdk::port::utils

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_ */
