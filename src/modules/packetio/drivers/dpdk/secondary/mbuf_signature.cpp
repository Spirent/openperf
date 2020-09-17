#include "core/op_log.h"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"

namespace openperf::packetio::dpdk {

size_t mbuf_signature_offset = 0;
uint64_t mbuf_signature_flag = 0;

void mbuf_signature_init()
{
    auto offset = rte_mbuf_dynfield_register(&mbuf_dynfield_signature);
    if (offset < 0) {
        throw std::runtime_error("Could not find dynamic field for signature: "
                                 + std::string(rte_strerror(rte_errno)));
    }

    mbuf_signature_offset = offset;

    auto bitnum = rte_mbuf_dynflag_register(&mbuf_dynflag_signature);
    if (bitnum < 0) {
        throw std::runtime_error("Could not find dynamic flag for signature: "
                                 + std::string(rte_strerror(rte_errno)));
    }

    mbuf_signature_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic signature field found: offset = %d, bitflag = %d\n",
           offset,
           bitnum);
}

} // namespace openperf::packetio::dpdk
