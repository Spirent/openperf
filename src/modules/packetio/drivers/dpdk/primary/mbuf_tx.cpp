#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"

namespace openperf::packetio::dpdk {

uint64_t mbuf_tx_sink_flag = 0;

void mbuf_tx_init()
{
    /* Register the tx sink mbuf flag for any available bit */
    auto bitnum = rte_mbuf_dynflag_register_bitnum(&mbuf_dynflag_tx, UINT_MAX);
    if (bitnum < 0) {
        throw std::runtime_error(
            "Could not register dynamic bit number for mbuf tx sink: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_tx_sink_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic mbuf Tx sink registered: bitflag = %d\n",
           bitnum);
}

} // namespace openperf::packetio::dpdk
