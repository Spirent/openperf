#ifndef _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_
#define _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_

#include <chrono>

namespace openperf::packetio::dpdk::port::utils {

/*
 * The timestamp in the Spirent signature field is 38 bits long and
 * contains 2.5ns ticks. Hence, we have 400000000 ticks per second
 * and need to mask out the lower 38 bits when calculating the
 * wall-clock offset.
 */
using phxtime = std::chrono::duration<int64_t, std::ratio<1, 400000000>>;

template <typename Clock>
inline phxtime get_phxtime_offset(const std::chrono::seconds epoch_offset)
{
    static constexpr uint64_t offset_mask = 0xffffffc000000000;
    auto now = Clock::now();

    auto phx_offset = std::chrono::duration_cast<phxtime>(now.time_since_epoch()
                                                          - epoch_offset);
    auto phx_fudge = phxtime{phx_offset.count() & offset_mask};
    return (epoch_offset + phx_fudge);
}

/*
 * Spirent signature fields use timestamps epochs that begin with
 * the current year.  This function calculates the proper offset
 * from the UNIX epoch.
 */
time_t get_timestamp_epoch_offset();

} // namespace openperf::packetio::dpdk::port::utils

#endif /* _OP_PACKETIO_DPDK_PORT_SIGNATURE_UTILS_HPP_ */
