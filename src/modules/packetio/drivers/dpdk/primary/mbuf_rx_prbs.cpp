#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/mbuf_rx_prbs.hpp"

namespace openperf::packetio::dpdk {

uint64_t mbuf_rx_prbs_flag = 0;

void mbuf_rx_prbs_init()
{
    static constexpr auto mbuf_dynflag_rx_prbs =
        rte_mbuf_dynflag{.name = "packetio_dynflag_rx_prbs", .flags = 0};

    auto bitnum =
        rte_mbuf_dynflag_register_bitnum(&mbuf_dynflag_rx_prbs, UINT_MAX);
    if (bitnum < 0) {
        throw std::runtime_error(
            "Could not register dynamic bit number of rx PRBS data: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_rx_prbs_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic Rx PRBS flag registered: bitflag = %d\n",
           bitnum);
}

} // namespace openperf::packetio::dpdk
