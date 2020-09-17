#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/mbuf_tx.hpp"

namespace openperf::packetio::dpdk {

uint64_t mbuf_tx_sink_flag = 0;

void mbuf_tx_init()
{
    auto bitnum = rte_mbuf_dynflag_register(&mbuf_dynflag_tx);
    if (bitnum < 0) {
        throw std::runtime_error(
            "Could not find dynamic flag for mbuf tx sink: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_tx_sink_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic mbuf Tx sink flag found: bitflag = %d\n",
           bitnum);
}

} // namespace openperf::packetio::dpdk
