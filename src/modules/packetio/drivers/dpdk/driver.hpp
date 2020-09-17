#ifndef _OP_PACKETIO_DPDK_DRIVER_HPP_
#define _OP_PACKETIO_DPDK_DRIVER_HPP_

#include <vector>
#include <optional>
#include <string>

#include "packetio/generic_driver.hpp"
#include "packetio/generic_port.hpp"

namespace openperf::packetio {

namespace port {
class generic_port;
}

namespace dpdk {

template <typename ProcessType> class driver
{
public:
    driver();
    ~driver();

    /*
     * This object is effectively a singleton; we can only initialize
     * DPDK once. So allow moves, but not copies.
     */
    driver(driver&& other) noexcept;
    driver& operator=(driver&& other) noexcept;

    driver(const driver&) = delete;
    driver& operator=(const driver&) = delete;

    /* Generic driver hooks */
    std::vector<std::string> port_ids() const;
    std::optional<port::generic_port> port(std::string_view id) const;
    std::optional<int> port_index(std::string_view id) const;

    tl::expected<std::string, std::string>
    create_port(std::string_view id, const port::config_data& config);
    tl::expected<void, std::string> delete_port(std::string_view id);

    void start_all_ports();
    void stop_all_ports();

    using port_id_map = std::map<uint16_t, std::string>;

private:
    std::unique_ptr<ProcessType> m_process;
    port_id_map m_ethdev_ports;
    port_id_map m_bonded_ports;
};

} // namespace dpdk
} // namespace openperf::packetio

#endif /* _OP_PACKETIO_DPDK_DRIVER_HPP_ */
