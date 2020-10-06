#ifndef _OP_PACKETIO_DPDK_PRIMARY_EAL_PROCESS_HPP_
#define _OP_PACKETIO_DPDK_PRIMARY_EAL_PROCESS_HPP_

#include <memory>

#include "packetio/generic_port.hpp"
#include "packetio/memory/dpdk/primary/pool_allocator.hpp"

struct rte_ring;

namespace openperf::packetio::dpdk::primary {

template <typename ProcessType> class eal_process
{
public:
    eal_process();
    ~eal_process();

    std::optional<port::generic_port> get_port(uint16_t port_idx,
                                               std::string_view id) const;
    void start_port(uint16_t port_idx);
    void stop_port(uint16_t port_idx);
};

class live_port_process : public eal_process<live_port_process>
{
public:
    static void do_port_callbacks();
    static void do_cleanup();
};

class test_port_process : public eal_process<test_port_process>
{
public:
    static void do_port_setup();
};

} // namespace openperf::packetio::dpdk::primary

#endif /* _OP_PACKETIO_DPDK_PRIMARY_EAL_PROCESS_HPP_ */
