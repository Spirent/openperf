#ifndef _OP_PACKETIO_DPDK_TX_SOURCE_HPP_
#define _OP_PACKETIO_DPDK_TX_SOURCE_HPP_

/**
 * @file
 *
 * A DPDK tx_source is just a wrapper around a generic packet_source that
 * associates a DPDK mbuf memory pool with the source.
 * The generic source needs the packets to do it's job.
 */

#include <memory>

#include "packetio/generic_source.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/packet_pool.hpp"

namespace openperf::packetio::dpdk {

class tx_source
{
    mutable packet_pool m_pool;
    packet::generic_source m_source;

public:
    tx_source(uint16_t port_idx, packet::generic_source source);
    ~tx_source() = default;

    std::string id() const;
    bool active() const;
    uint16_t burst_size() const;
    uint16_t max_packet_length() const;
    packet::packets_per_hour packet_rate() const;
    uint16_t pull(rte_mbuf* packets[], uint16_t count) const;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_TX_SOURCE_HPP_ */
