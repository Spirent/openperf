#ifndef _OP_PACKETIO_DPDK_PRIMARY_PHYSICAL_PORT_HPP_
#define _OP_PACKETIO_DPDK_PRIMARY_PHYSICAL_PORT_HPP_

#include "packetio/drivers/dpdk/physical_port.tcc"

namespace openperf::packetio::dpdk::primary {

class physical_port final
    : public openperf::packetio::dpdk::physical_port<physical_port>
{
    rte_mempool* const m_mempool;

public:
    physical_port(uint16_t idx,
                  std::string_view id,
                  rte_mempool* const mempool);

    tl::expected<void, std::string> do_config(uint16_t port_idx,
                                              const port::config_data& config);
};

} // namespace openperf::packetio::dpdk::primary

#endif /* _OP_PACKETIO_DPDK_PRIMARY_PHYSICAL_PORT_HPP_ */
