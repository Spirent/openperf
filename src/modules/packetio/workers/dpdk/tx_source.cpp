#include <array>

#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/workers/dpdk/tx_source.h"
#include "packetio/workers/dpdk/worker_api.h"

namespace icp::packetio::dpdk {

static packet_pool make_packet_pool(uint16_t port_idx, const packets::generic_source& source)
{
    auto info = model::port_info(port_idx);
    return (packet_pool(source.id(),
                        info.socket_id(),
                        source.max_packet_length(),
                        512));  /* XXX: hard coded for testing */
}

tx_source::tx_source(uint16_t port_idx, packets::generic_source source)
    : m_pool(make_packet_pool(port_idx, source))
    , m_source(std::move(source))
{}

std::string tx_source::id() const
{
    return (m_source.id());
}

bool tx_source::active() const
{
    return (m_source.active());
}

uint16_t tx_source::burst_size() const
{
    return (m_source.burst_size());
}

uint16_t tx_source::max_packet_length() const
{
    return (m_source.max_packet_length());
}

packets::packets_per_hour tx_source::packet_rate() const
{
    return (m_source.packet_rate());
}

uint16_t tx_source::pull(rte_mbuf* packets[], uint16_t count) const
{
    uint16_t nb_packets = 0;
    std::array<packets::packet_buffer*, worker::pkt_burst_size> tmp;

    while (nb_packets < count) {
        auto n = m_pool.get(tmp.data(), std::min(static_cast<int>(tmp.size()),
                                                 count - nb_packets));
        if (!n) break;

        auto m = m_source.transform(tmp.data(), n,
                                    reinterpret_cast<packets::packet_buffer**>(packets) + nb_packets);
        nb_packets += m;

        if (m < n) {
            /* Free any unused packet buffers */
            m_pool.put(tmp.data() + m, n - m);
            break;
        }
    }

    return (nb_packets);
}

}
