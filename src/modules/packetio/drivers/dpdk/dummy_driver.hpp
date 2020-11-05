#ifndef _OP_PACKETIO_DPDK_DUMMY_DRIVER_HPP_
#define _OP_PACKETIO_DPDK_DUMMY_DRIVER_HPP_

#include <string>
#include <vector>

#include "packetio/generic_port.hpp"

namespace openperf::packetio::dpdk {

class dummy_driver
{
public:
    std::vector<std::string> port_ids() const
    {
        return (std::vector<std::string>{});
    }

    std::optional<port::generic_port> port(std::string_view) const
    {
        return (std::nullopt);
    }

    std::optional<int> port_index(std::string_view) const
    {
        return (std::nullopt);
    }

    tl::expected<std::string, std::string> create_port(std::string_view,
                                                       const port::config_data&)
    {
        return (
            tl::make_unexpected("Dummy driver does not support port creation"));
    }

    tl::expected<void, std::string> delete_port(std::string_view)
    {
        return (
            tl::make_unexpected("Dummy driver does not support port deletion"));
    }

    void start_all_ports() {}

    void stop_all_ports() {}

    bool is_usable() { return (false); }
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_DPDK_DUMMY_DRIVER_HPP_ */
