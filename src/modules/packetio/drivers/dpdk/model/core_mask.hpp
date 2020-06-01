#ifndef _OP_PACKETIO_DPDK_MODEL_CORE_MASK_HPP_
#define _OP_PACKETIO_DPDK_MODEL_CORE_MASK_HPP_

#include <bitset>
#include <sstream>

#include "packetio/drivers/dpdk/dpdk.h"

namespace openperf::packetio::dpdk::model {

using core_mask = std::bitset<RTE_MAX_LCORE>;

inline std::string to_string(const core_mask& mask)
{
    std::ostringstream out;
    out << "0x" << std::hex << mask.to_ulong();
    return (out.str());
}

} // namespace openperf::packetio::dpdk::model

#endif /* _OP_PACKETIO_DPDK_MODEL_CORE_MASK_HPP_ */
