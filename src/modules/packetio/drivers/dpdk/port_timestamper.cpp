#include <chrono>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/port_timestamper.hpp"
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
    return (link.link_status == ETH_LINK_UP
                ? link.link_speed
                : model::port_info(port_id).max_speed());
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

    std::for_each(packets, packets + nb_packets, [&](auto& m) {
        m->ol_flags |= PKT_RX_TIMESTAMP;
        m->timestamp = (now
                        + std::chrono::duration_cast<nanoseconds>(
                            rx_octets * ps_per_octet))
                           .count();
        rx_octets += rte_pktmbuf_pkt_len(m) + ethernet_overhead;
    });

    return (nb_packets);
}

callback_timestamper::callback_timestamper(uint16_t port_id)
    : m_port(port_id)
{}

callback_timestamper::~callback_timestamper()
{
    if (m_callbacks.empty()) return;

    std::for_each(
        std::begin(m_callbacks), std::end(m_callbacks), [&](auto& item) {
            rte_eth_remove_rx_callback(port_id(), item.first, item.second);
        });
}

callback_timestamper::callback_timestamper(
    callback_timestamper&& other) noexcept
    : m_callbacks(std::move(other.m_callbacks))
    , m_port(other.m_port)
{}

callback_timestamper&
callback_timestamper::operator=(callback_timestamper&& other) noexcept
{
    if (this != &other) {
        m_callbacks = std::move(other.m_callbacks);
        m_port = other.m_port;
    }

    return (*this);
}

uint16_t callback_timestamper::port_id() const { return (m_port); }

void callback_timestamper::enable()
{
    OP_LOG(OP_LOG_INFO,
           "Enabling software packet timestamps on port %u\n",
           m_port);

    for (auto q = 0U; q < model::port_info(port_id()).rx_queue_count(); q++) {
        auto cb = rte_eth_add_first_rx_callback(
            port_id(), q, timestamp_packets, nullptr);
        if (!cb) {
            throw std::runtime_error("Could not add timestamp callback");
        }
        m_callbacks.emplace(q, cb);
    }
}

void callback_timestamper::disable()
{
    OP_LOG(OP_LOG_INFO,
           "Disabling software packet timestamps on port %u\n",
           m_port);

    std::for_each(
        std::begin(m_callbacks), std::end(m_callbacks), [&](auto& item) {
            rte_eth_remove_rx_callback(port_id(), item.first, item.second);
        });
}

static timestamper::variant_type make_timestamper(uint16_t port_id)
{
    return (callback_timestamper(port_id));
}

timestamper::timestamper(uint16_t port_id)
    : feature(make_timestamper(port_id))
{}

} // namespace openperf::packetio::dpdk::port
