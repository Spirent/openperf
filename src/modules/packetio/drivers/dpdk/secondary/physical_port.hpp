#ifndef _OP_PACKETIO_DPDK_SECONDARY_PHYSICAL_PORT_HPP_
#define _OP_PACKETIO_DPDK_SECONDARY_PHYSICAL_PORT_HPP_

#include "packetio/drivers/dpdk/physical_port.tcc"

namespace openperf::packetio::dpdk::secondary {

class physical_port final
    : public openperf::packetio::dpdk::physical_port<physical_port>
{
public:
    physical_port(uint16_t idx, std::string_view id)
        : dpdk::physical_port<physical_port>(idx, id)
    {}

    tl::expected<void, std::string> do_config(uint16_t idx,
                                              const port::config_data&)
    {
        return (tl::make_unexpected("Cannot configure port "
                                    + std::to_string(idx)
                                    + " from secondary process"));
    }
};

} // namespace openperf::packetio::dpdk::secondary

#endif /* _OP_PACKETIO_DPDK_SECONDARY_PHYSICAL_PORT_HPP_ */
