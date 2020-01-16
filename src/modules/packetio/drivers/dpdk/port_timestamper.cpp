#include <chrono>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/drivers/dpdk/port_timestamper.hpp"
#include "timesync/chrono.hpp"
#include "units/data-rates.hpp"

namespace openperf::packetio::dpdk::port {

static uint16_t timestamp_packets(uint16_t port_id,
                                  [[maybe_unused]] uint16_t queue_id,
                                  rte_mbuf* packets[],
                                  uint16_t nb_packets,
                                  [[maybe_unused]] uint16_t max_packets,
                                  [[maybe_unused]] void* user_param)
{
    using namespace openperf::units;
    using clock = openperf::timesync::chrono::realtime;

    static constexpr auto ethernet_bytes = 8U;
    static constexpr auto ethernet_overhead = 20U;
    static constexpr auto ethernet_quanta = 64U;

    /* Find the current link speed */
    struct rte_eth_link link;
    rte_eth_link_get_nowait(port_id, &link);

    /*
     * Calculate nanoseconds per quanta;
     * nanoseconds per byte is less than 1 nanosecond at 100G speeds!
     * (and hence, too small for us to use)
     */
    using bps = rate<uint64_t>;
    using mbps = rate<uint64_t, megabits>;

    auto ns_per_quanta = to_duration<std::chrono::nanoseconds>(
        rate_cast<bps>(mbps(link.link_speed))
        / (ethernet_bytes * ethernet_quanta));

    /*
     * Iterate through the packet array in reverse order.  For each packet,
     * increase the offset by the transmit time for that packet.
     * This allows us to calculate the maximum realistic receive timestamp
     * for each packet.
     */
    auto now = clock::now().time_since_epoch();
    auto offset = clock::duration::zero();

    std::for_each(std::make_reverse_iterator(packets + nb_packets),
                  std::make_reverse_iterator(packets),
                  [&](auto& m) {
                      m->timestamp = (now - offset).count();
                      offset += ns_per_quanta
                                * (rte_pktmbuf_pkt_len(m) + ethernet_overhead)
                                / ethernet_quanta;
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

callback_timestamper::callback_timestamper(callback_timestamper&& other)
    : m_callbacks(std::move(other.m_callbacks))
    , m_port(other.m_port)
{}

callback_timestamper&
callback_timestamper::operator=(callback_timestamper&& other)
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

null_timestamper::null_timestamper(uint16_t port_id)
    : m_port(port_id)
{}

uint16_t null_timestamper::port_id() const { return (m_port); }

void null_timestamper::enable() {}
void null_timestamper::disable() {}

static timestamper::timestamper_strategy make_timestamper(uint16_t port_id)
{
    return (callback_timestamper(port_id));
}

timestamper::timestamper(uint16_t port_id)
    : m_timestamper(make_timestamper(port_id))
{}

uint16_t timestamper::port_id() const
{
    auto id_visitor = [](auto& stamper) { return stamper.port_id(); };
    return (std::visit(id_visitor, m_timestamper));
}

void timestamper::enable()
{
    auto enable_visitor = [](auto& stamper) { return stamper.enable(); };
    std::visit(enable_visitor, m_timestamper);
}

void timestamper::disable()
{
    auto disable_visitor = [](auto& stamper) { return stamper.disable(); };
    std::visit(disable_visitor, m_timestamper);
}

} // namespace openperf::packetio::dpdk::port
