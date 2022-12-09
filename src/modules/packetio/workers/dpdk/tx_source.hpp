#ifndef _OP_PACKETIO_DPDK_TX_SOURCE_HPP_
#define _OP_PACKETIO_DPDK_TX_SOURCE_HPP_

/**
 * @file
 *
 * A DPDK tx_source is just a wrapper around a generic packet_source that
 * associates a DPDK mbuf memory pool with the source.
 * The generic source needs the packets to do it's job.
 */

#include "packetio/generic_source.hpp"
#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::dpdk {

class tx_source
{
    mutable rte_mempool* m_pool;
    packet::generic_source m_source;

public:
    tx_source(packet::generic_source source, struct rte_mempool* pool);

    tx_source(tx_source&& other) noexcept;
    tx_source& operator=(tx_source&& other) noexcept;

    tx_source(const tx_source&) = delete;
    tx_source& operator=(const tx_source&&) = delete;

    ~tx_source() = default;

    std::string id() const;
    bool active() const;
    bool uses_feature(packet::source_feature_flags flags) const;
    uint16_t burst_size() const;
    uint16_t max_packet_length() const;
    packet::packets_per_hour packet_rate() const;
    uint16_t pull(rte_mbuf* packets[], uint16_t count) const;
    void update_drop_counters(uint16_t packets, size_t octets) const;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_TX_SOURCE_HPP_ */
