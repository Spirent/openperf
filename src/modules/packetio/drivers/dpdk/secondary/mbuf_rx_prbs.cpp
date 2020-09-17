#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/mbuf_rx_prbs.hpp"

namespace openperf::packetio::dpdk {

uint64_t mbuf_rx_prbs_flag = 0;

void mbuf_rx_prbs_init()
{
    auto bitnum = rte_mbuf_dynflag_register(&mbuf_dynflag_rx_prbs);
    if (bitnum < 0) {
        throw std::runtime_error(
            "Could not find dynamic flag for Rx PRBS data: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_rx_prbs_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG, "Dynamic Rx PRBS field found: bitflag = %d\n", bitnum);
}

} // namespace openperf::packetio::dpdk
