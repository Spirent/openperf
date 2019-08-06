#ifndef _ICP_PACKETIO_DPDK_TX_SOURCE_H_
#define _ICP_PACKETIO_DPDK_TX_SOURCE_H_

/**
 * @file
 *
 * A DPDK tx_source is just a wrapper around a generic packet_source that
 * associates a DPDK mbuf memory pool with the source.
 * The generic source needs the packets to do it's job.
 */

#include <memory>

#include "packetio/generic_source.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/packet_pool.h"

namespace icp::packetio::dpdk {

class tx_source
{
    mutable packet_pool m_pool;
    packets::generic_source m_source;

public:
    tx_source(uint16_t port_idx, packets::generic_source source);
    ~tx_source() = default;

    std::string id() const;
    bool active() const;
    uint16_t burst_size() const;
    uint16_t max_packet_length() const;
    packets::packets_per_hour packet_rate() const;
    uint16_t pull(rte_mbuf* packets[], uint16_t count) const;
};

}

#endif /* _ICP_PACKETIO_DPDK_TX_SOURCE_H_ */
