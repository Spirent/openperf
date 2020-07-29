#ifndef _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_
#define _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_

#include "packetio/drivers/dpdk/dpdk.h"
#include <algorithm>

namespace openperf::packetio::dpdk {

/** mbuf ol_flag used to indicate the mbuf should be passed to tx sink. */
extern uint64_t mbuf_tx_sink_flag;

/** Set mbuf ol_flag indicating the mbuf should be passed to tx sink. */
inline void mbuf_set_tx_sink(rte_mbuf* mbuf)
{
    mbuf->ol_flags |= mbuf_tx_sink_flag;
}

/** Clear mbuf ol_flag indicating the mbuf should be passed to tx sink.
 */
inline void mbuf_clear_tx_sink(rte_mbuf* mbuf)
{
    mbuf->ol_flags &= (~mbuf_tx_sink_flag);
}

/** Get mbuf ol_flag indicating mbuf should be pased to tx sink */
inline bool mbuf_tx_sink(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_tx_sink_flag);
}

/**
 * Store the hardware address of the interface used for transmit in the mbuf.
 *
 * The hardware address is stored in the mbuf user data and should be considered
 * transient.  It is only valid in certain parts of the transmit path.
 */
inline void mbuf_set_tx_hwaddr(rte_mbuf* mbuf, const uint8_t hwaddr[])
{
    auto dst = reinterpret_cast<uint8_t*>(&mbuf->udata64);
    std::copy_n(hwaddr, RTE_ETHER_ADDR_LEN, dst);
}

/**
 * Get the the interface hardware address used for transmit.
 *
 * The hardware address is stored in the mbuf user data and should be considered
 * transient.  It is only valid in certain parts of the transmit path.
 */
inline void mbuf_get_tx_hwaddr(const rte_mbuf* mbuf, uint8_t hwaddr[])
{
    auto src = reinterpret_cast<const uint8_t*>(&mbuf->udata64);
    std::copy_n(src, RTE_ETHER_ADDR_LEN, hwaddr);
}

void mbuf_tx_init();

} // namespace openperf::packetio::dpdk

#endif // _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_