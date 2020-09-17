#include "core/op_log.h"
#include "packetio/drivers/dpdk/secondary/eal_process.hpp"
#include "packetio/drivers/dpdk/secondary/physical_port.hpp"

namespace openperf::packetio::dpdk::secondary {

std::optional<port::generic_port> eal_process::get_port(uint16_t port_idx,
                                                        std::string_view id)
{
    return (physical_port(port_idx, id));
}

void eal_process::start_port(uint16_t port_idx)
{
    OP_LOG(OP_LOG_DEBUG, "Ignoring request to start port %u\n", port_idx);
}

void eal_process::stop_port(uint16_t port_idx)
{
    OP_LOG(OP_LOG_DEBUG, "Ignoring request to stop port %u\n", port_idx);
}

} // namespace openperf::packetio::dpdk::secondary
