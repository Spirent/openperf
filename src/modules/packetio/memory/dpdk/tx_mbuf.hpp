#ifndef _OP_PACKETIO_MEMORY_DPDK_TX_MBUF_HPP_
#define _OP_PACKETIO_MEMORY_DPDK_TX_MBUF_HPP_

#include "packetio/drivers/dpdk/dpdk.h"
#include <algorithm>

namespace openperf::packetio::dpdk {

inline void tx_mbuf_set_hwaddr(rte_mbuf* mbuf, const uint8_t hwaddr[])
{
    auto dst = reinterpret_cast<uint8_t*>(&mbuf->udata64);
    std::copy_n(hwaddr, RTE_ETHER_ADDR_LEN, dst);
}

inline void tx_mbuf_get_hwaddr(const rte_mbuf* mbuf, uint8_t hwaddr[])
{
    auto src = reinterpret_cast<const uint8_t*>(&mbuf->udata64);
    std::copy_n(src, RTE_ETHER_ADDR_LEN, hwaddr);
}

} // namespace openperf::packetio::dpdk

#endif // _OP_PACKETIO_MEMORY_DPDK_TX_MBUF_HPP_