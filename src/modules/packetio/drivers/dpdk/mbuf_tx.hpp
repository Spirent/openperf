#ifndef _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_
#define _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_

#include "packetio/drivers/dpdk/dpdk.h"
#include <algorithm>

namespace openperf::packetio::dpdk {

/** mbuf ol_flag used to indicate the mbuf is in the transmit direction. */
extern uint64_t mbuf_tx_flag;

/** Set mbuf ol_flag indicating the mbuf is going in the transmit direction. */
inline void mbuf_tx_set(rte_mbuf* mbuf) { mbuf->ol_flags |= mbuf_tx_flag; }

/** Clear mbuf ol_flag indicating the mbuf is going in the transmit direction.
 */
inline void mbuf_tx_clear(rte_mbuf* mbuf) { mbuf->ol_flags &= (~mbuf_tx_flag); }

/** Get mbuf ol_flag indicating mbuf is going in the transmit direction */
inline bool mbuf_tx_value(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_tx_flag);
}

/**
 * Store the hardware address of the interface used for transmit in the mbuf.
 *
 * The hardware address is stored in the mbuf user data and should be considered
 * transient.  It is only valid in certain parts of the transmit path.
 */
inline void mbuf_tx_set_hwaddr(rte_mbuf* mbuf, const uint8_t hwaddr[])
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
inline void mbuf_tx_get_hwaddr(const rte_mbuf* mbuf, uint8_t hwaddr[])
{
    auto src = reinterpret_cast<const uint8_t*>(&mbuf->udata64);
    std::copy_n(src, RTE_ETHER_ADDR_LEN, hwaddr);
}

void mbuf_tx_init();

} // namespace openperf::packetio::dpdk

#endif // _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_