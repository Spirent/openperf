#ifndef _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_
#define _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_

#include "packetio/drivers/dpdk/dpdk.h"
#include <algorithm>

namespace openperf::packetio::dpdk {

extern uint64_t mbuf_tx_flag;

inline void mbuf_tx_set(rte_mbuf* mbuf) { mbuf->ol_flags |= mbuf_tx_flag; }

inline void mbuf_tx_clear(rte_mbuf* mbuf) { mbuf->ol_flags &= (~mbuf_tx_flag); }

inline bool mbuf_tx_value(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_tx_flag);
}

inline void mbuf_tx_set_hwaddr(rte_mbuf* mbuf, const uint8_t hwaddr[])
{
    auto dst = reinterpret_cast<uint8_t*>(&mbuf->udata64);
    std::copy_n(hwaddr, RTE_ETHER_ADDR_LEN, dst);
}

inline void mbuf_tx_get_hwaddr(const rte_mbuf* mbuf, uint8_t hwaddr[])
{
    auto src = reinterpret_cast<const uint8_t*>(&mbuf->udata64);
    std::copy_n(src, RTE_ETHER_ADDR_LEN, hwaddr);
}

void mbuf_tx_init();

} // namespace openperf::packetio::dpdk

#endif // _OP_PACKETIO_DRIVERS_DPDK_MBUF_TX_HPP_