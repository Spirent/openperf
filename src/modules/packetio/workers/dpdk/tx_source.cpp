#include <array>

#include "packetio/drivers/dpdk/port_info.hpp"
#include "packetio/memory/dpdk/mempool.hpp"
#include "packetio/workers/dpdk/tx_source.hpp"
#include "packetio/workers/dpdk/worker_api.hpp"

namespace openperf::packetio::dpdk {

tx_source::tx_source(packet::generic_source source, struct rte_mempool* pool)
    : m_pool(pool)
    , m_source(std::move(source))
{
    if (!m_pool) {
        throw std::runtime_error("No packet pool available for source "
                                 + m_source.id());
    }
}

tx_source::tx_source(tx_source&& other) noexcept
    : m_pool(other.m_pool)
    , m_source(std::move(other.m_source))
{}

tx_source& tx_source::operator=(tx_source&& other) noexcept
{
    if (this != &other) {
        m_pool = other.m_pool;
        m_source = std::move(other.m_source);
    }
    return (*this);
}

std::string tx_source::id() const { return (m_source.id()); }

bool tx_source::active() const { return (m_source.active()); }

bool tx_source::uses_feature(packet::source_feature_flags flags) const
{
    return (m_source.uses_feature(flags));
}

uint16_t tx_source::burst_size() const { return (m_source.burst_size()); }

uint16_t tx_source::max_packet_length() const
{
    return (m_source.max_packet_length());
}

packet::packets_per_hour tx_source::packet_rate() const
{
    return (m_source.packet_rate());
}

uint16_t tx_source::pull(rte_mbuf* packets[], uint16_t count) const
{

    std::array<rte_mbuf*, worker::pkt_burst_size> tmp;
    assert(count <= tmp.size());

    auto n = std::min(static_cast<uint16_t>(tmp.size()), count);
    auto error = rte_pktmbuf_alloc_bulk(m_pool, tmp.data(), n);
    if (error) {
        OP_LOG(OP_LOG_WARNING,
               "No packet buffers available for %s: %s\n",
               id().c_str(),
               rte_strerror(std::abs(error)));
        return (0);
    }

    auto m = m_source.transform(
        reinterpret_cast<packet::packet_buffer**>(tmp.data()),
        n,
        reinterpret_cast<packet::packet_buffer**>(packets));

    /* Free any unused packet buffers */
    if (m < n) {
        std::for_each(tmp.data() + m, tmp.data() + n, rte_pktmbuf_free);
    };

    return (m);
}

} // namespace openperf::packetio::dpdk
