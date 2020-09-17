#ifndef _OP_PACKETIO_DPDK_SECONDARY_EAL_PROCESS_HPP_
#define _OP_PACKETIO_DPDK_SECONDARY_EAL_PROCESS_HPP_

#include "packetio/generic_port.hpp"

namespace openperf::packetio::dpdk::secondary {

class eal_process
{
public:
    static std::optional<port::generic_port> get_port(uint16_t port_idx,
                                                      std::string_view id);

    static void start_port(uint16_t port_idx);
    static void stop_port(uint16_t port_idx);
};

} // namespace openperf::packetio::dpdk::secondary

#endif /* _OP_PACKETIO_DPDK_SECONDARY_EAL_PROCESS_HPP_ */
