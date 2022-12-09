#include <stdexcept>
#include <string>

#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_metadata.hpp"

namespace openperf::packetio::dpdk {

size_t mbuf_metadata_offset = 0;
uint64_t mbuf_prbs_data_flag = 0;
uint64_t mbuf_signature_flag = 0;
uint64_t mbuf_tagged_flag = 0;
uint64_t mbuf_timestamp_flag = 0;
uint64_t mbuf_tx_sink_flag = 0;

/** mbuf flag used to indicate the presence of PRBS data; rx only */
constexpr auto mbuf_dynflag_prbs_data =
    rte_mbuf_dynflag{.name = "packetio_dynflag_prbs_data", .flags = 0};

/** mbuf flag used to indicate the presence of signature data */
constexpr auto mbuf_dynflag_signature =
    rte_mbuf_dynflag{.name = "packetio_dynflag_signature", .flags = 0};

/** mbuf flag used to indicate the presence of an interface tag */
constexpr auto mbuf_dynflag_tagged =
    rte_mbuf_dynflag{.name = "packetio_dynflag_tagged", .flags = 0};

/** mbuf flag used to indicate the presence of a timestamp */
constexpr auto mbuf_dynflag_timestamp =
    rte_mbuf_dynflag{.name = "packetio_dynflag_timestamp", .flags = 0};

/** mbuf flag used to indicate the mbuf should be passed to tx sinks; tx only */
constexpr auto mbuf_dynflag_tx_sink =
    rte_mbuf_dynflag{.name = "packetio_dynflag_tx_sink", .flags = 0};

constexpr auto mbuf_dynfield_metadata =
    rte_mbuf_dynfield{.name = "packetio_dynfield_metadata",
                      .size = sizeof(mbuf_metadata),
                      .align = __alignof(uint64_t),
                      .flags = 0};

static uint64_t register_mbuf_dynflag(const rte_mbuf_dynflag& info)
{
    auto bitnum = rte_mbuf_dynflag_register(&info);
    if (bitnum < 0) {
        throw std::runtime_error("Could not register dynamic bit number for "
                                 + std::string(info.name) + ": "
                                 + std::string(rte_strerror(rte_errno)));
    }

    return (1ULL << bitnum);
}

void mbuf_metadata_init()
{
    mbuf_prbs_data_flag = register_mbuf_dynflag(mbuf_dynflag_prbs_data);
    mbuf_signature_flag = register_mbuf_dynflag(mbuf_dynflag_signature);
    mbuf_tagged_flag = register_mbuf_dynflag(mbuf_dynflag_tagged);
    mbuf_timestamp_flag = register_mbuf_dynflag(mbuf_dynflag_timestamp);
    mbuf_tx_sink_flag = register_mbuf_dynflag(mbuf_dynflag_tx_sink);

    auto offset = rte_mbuf_dynfield_register(&mbuf_dynfield_metadata);
    if (offset < 0) {
        throw std::runtime_error(
            "Could not regsiter dynamic field for metadata: "
            + std::string(rte_strerror(rte_errno)));
    }

    mbuf_metadata_offset = offset;

    OP_LOG(OP_LOG_DEBUG,
           "Dynamic metadata registered: %zu bytes at offset = %d"
           "; PRBS = 0x%" PRIx64 ", signature = 0x%" PRIx64
           ", tagged = 0x%" PRIx64 ", timestamp = 0x%" PRIx64
           ", tx sink = 0x%" PRIx64 "\n",
           sizeof(mbuf_metadata),
           offset,
           mbuf_prbs_data_flag,
           mbuf_signature_flag,
           mbuf_tagged_flag,
           mbuf_timestamp_flag,
           mbuf_tx_sink_flag);
}

} // namespace openperf::packetio::dpdk
