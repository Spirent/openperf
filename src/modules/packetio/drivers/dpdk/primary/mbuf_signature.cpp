#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"

namespace openperf::packetio::dpdk {

size_t mbuf_signature_offset = 0;
uint64_t mbuf_signature_flag = 0;

void mbuf_signature_init()
{
    auto offset = rte_mbuf_dynfield_register(&mbuf_dynfield_signature);
    if (offset < 0) {
        throw std::runtime_error(
            "Could not register dynamic field for signature: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_signature_offset = offset;

    /* Register the signature flag for any available bit */
    auto bitnum =
        rte_mbuf_dynflag_register_bitnum(&mbuf_dynflag_signature, UINT_MAX);
    if (bitnum < 0) {
        throw std::runtime_error(
            "Could not register dynamic bit number for signature: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_signature_flag = (1ULL << bitnum);

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic signature field registered: offset = %d, bitflag = %d\n",
           offset,
           bitnum);
};

} // namespace openperf::packetio::dpdk
