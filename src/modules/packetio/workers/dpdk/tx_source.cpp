#include <array>

#include "packetio/drivers/dpdk/model/port_info.hpp"
#include "packetio/workers/dpdk/tx_source.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"

namespace openperf::packetio::dpdk {

static packet_pool make_packet_pool(uint16_t port_idx, const packets::generic_source& source)
{
    auto info = model::port_info(port_idx);

    /*
     * A note about pool and cache sizing: we want to make sure we have packets
     * available if the NIC's transmit ring is completely full, so we need our
     * pool size to be a little larger than the ring size.  Similarly, we need
     * the cache to be larger than the worker's standard transaction size,
     * otherwise every call to retrieve buffers from the pool will bypass the
     * CPU cache and go straight to the pool.
     */
    return (packet_pool(source.id(),
                        info.socket_id(),
                        source.max_packet_length(),
                        info.tx_desc_count() + 2 * worker::pkt_burst_size,
                        2 * worker::pkt_burst_size));
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

    std::array<packets::packet_buffer*, worker::pkt_burst_size> tmp;
    assert(count <= tmp.size());

    auto n = m_pool.get(tmp.data(), std::min(static_cast<uint16_t>(tmp.size()), count));

    if (!n) {
        //OP_LOG(OP_LOG_WARNING, "No packet buffers available for %s\n", id().c_str());
        return (0);
    }

    auto m = m_source.transform(tmp.data(), n,
                                reinterpret_cast<packets::packet_buffer**>(packets));

    /* Free any unused packet buffers */
    if (m < n) m_pool.put(tmp.data() + m, n - m);

    return (m);
}

}
