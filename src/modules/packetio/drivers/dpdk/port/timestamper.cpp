#include <chrono>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_metadata.hpp"
#include "packetio/drivers/dpdk/port/timestamper.hpp"
#include "packetio/drivers/dpdk/port_info.hpp"
#include "timesync/chrono.hpp"
#include "units/data-rates.hpp"

namespace openperf::packetio::dpdk::port {

static uint32_t get_link_speed_safe(uint16_t port_id)
{
    /* Query the port's link speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);

    /*
     * Check the link status.  Sometimes, we get packets before
     * the port internals realize the link is up.  If that's the
     * case, then return the maximum speed of the port as a
     * safe default.
     */
    return (link.link_status == ETH_LINK_UP ? link.link_speed
                                            : port_info::max_speed(port_id));
}

static uint16_t timestamp_packets(uint16_t port_id,
                                  [[maybe_unused]] uint16_t queue_id,
                                  rte_mbuf* packets[],
                                  uint16_t nb_packets,
                                  [[maybe_unused]] uint16_t max_packets,
                                  [[maybe_unused]] void* user_param)
{
    using namespace openperf::units;
    using clock = openperf::timesync::chrono::realtime;

    /* Ethernet preamble + CRC */
    static constexpr auto ethernet_overhead = 24U;
    static constexpr auto ethernet_octets = 8U;

    /*
     * An octet takes less than 1 nanosecond at 100G speeds, so
     * calculate offsets in picoseconds.
     */
    using mbps = rate<uint64_t, megabits>;
    using nanoseconds = std::chrono::nanoseconds;
    using picoseconds = std::chrono::duration<int64_t, std::pico>;

    auto ps_per_octet =
        to_duration<picoseconds>(mbps(get_link_speed_safe(port_id)))
        * ethernet_octets;

    /*
     * Iterate through the packets. Assign each packet the current timestamp
     * plus the sum of the bit-times of all preceding packets.
     */
    auto now = clock::now().time_since_epoch();
    auto rx_octets = 0U;

    std::for_each(packets, packets + nb_packets, [&](auto* m) {
        mbuf_timestamp_set(m,
                           now
                               + std::chrono::duration_cast<nanoseconds>(
                                   rx_octets * ps_per_octet));
        rx_octets += rte_pktmbuf_pkt_len(m) + ethernet_overhead;
    });

    return (nb_packets);
}

std::string callback_timestamper::description() { return ("timestamping"); }

rx_callback<callback_timestamper>::rx_callback_fn
callback_timestamper::callback()
{
    return (timestamp_packets);
}

static timestamper::variant_type make_timestamper(uint16_t port_id)
{
    return (callback_timestamper(port_id));
}

timestamper::timestamper(uint16_t port_id)
    : feature(make_timestamper(port_id))
{}

} // namespace openperf::packetio::dpdk::port
