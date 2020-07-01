#ifndef _OP_PACKETIO_DPDK_MBUF_RX_PRBS_HPP_
#define _OP_PACKETIO_DPDK_MBUF_RX_PRBS_HPP_

#include <algorithm>
#include <cstdint>
#include <cstddef>

#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::dpdk {

/*
 * We store the PRBS data in the packets user data field.
 * TODO: Make this a proper dynamic field in the mbuf when
 * the dynamic field takes over the userdata field.
 */

struct mbuf_rx_prbs
{
    union
    {
        uint64_t prbs_data;
        struct
        {
            uint32_t octets;
            uint32_t bit_errors;
        };
    };
};

extern uint64_t mbuf_rx_prbs_flag;

void mbuf_rx_prbs_init();

inline void
mbuf_rx_prbs_set(rte_mbuf* mbuf, uint32_t octets, uint32_t bit_errors)
{
    mbuf->ol_flags |= mbuf_rx_prbs_flag;
    auto data = mbuf_rx_prbs{.octets = octets, .bit_errors = bit_errors};
    mbuf->udata64 = data.prbs_data;
}

inline bool mbuf_rx_prbs_avail(const rte_mbuf* mbuf)
{
    return (mbuf->ol_flags & mbuf_rx_prbs_flag);
}

inline uint32_t mbuf_rx_prbs_octets(const rte_mbuf* mbuf)
{
    return (reinterpret_cast<const mbuf_rx_prbs*>(&mbuf->udata64)->octets);
}

inline uint32_t mbuf_rx_prbs_bit_errors(const rte_mbuf* mbuf)
{
    return (reinterpret_cast<const mbuf_rx_prbs*>(&mbuf->udata64)->bit_errors);
}

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_MBUF_RX_PRBS_HPP_ */
