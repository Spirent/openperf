#ifndef _OP_PACKETIO_STACK_LWIP_HPP_
#define _OP_PACKETIO_STACK_LWIP_HPP_

#include <optional>
#include <string>
#include <map>
#include <vector>

#include "tl/expected.hpp"

#include "core/op_uuid.hpp"
#include "packetio/generic_interface.hpp"
#include "packetio/generic_stack.hpp"
#include "packetio/stack/dpdk/net_interface.hpp"

namespace openperf::packetio::dpdk {

class lwip
{
public:
    lwip(driver::generic_driver&, workers::generic_workers& workers);
    ~lwip();

    /* stack is movable */
    lwip& operator=(lwip&& other) noexcept;
    lwip(lwip&& other) noexcept;

    /* stack is non-copyable */
    lwip(const lwip&) = delete;
    lwip& operator=(const lwip&&) = delete;

    std::string id() const { return "stack0"; } /* only 1 stack for now */
    std::vector<std::string> interface_ids() const;
    std::optional<interface::generic_interface>
    interface(std::string_view id) const;

    tl::expected<std::string, std::string>
    create_interface(const interface::config_data& config);
    void delete_interface(std::string_view id);

    std::unordered_map<std::string, stack::stats_data> stats() const;

    using interface_map =
        std::map<std::string, std::unique_ptr<net_interface>, std::less<>>;

private:
    bool m_initialized;
    driver::generic_driver& m_driver;
    workers::generic_workers& m_workers;
    std::vector<std::string> m_tasks;
    interface_map m_interfaces;
    int m_timerfd;
};

} // namespace openperf::packetio::dpdk

#endif /* _OP_PACKETIO_STACK_LWIP_HPP_ */
