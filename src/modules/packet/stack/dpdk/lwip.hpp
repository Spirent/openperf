#ifndef _OP_PACKETIO_STACK_LWIP_HPP_
#define _OP_PACKETIO_STACK_LWIP_HPP_

#include <optional>
#include <string>
#include <map>
#include <vector>

#include "tl/expected.hpp"

#include "core/op_uuid.hpp"
#include "packetio/generic_interface.hpp"
#include "packet/stack/generic_stack.hpp"
#include "packet/stack/dpdk/net_interface.hpp"
#include "packetio/internal_client.hpp"

namespace openperf::packet::stack::dpdk {

class lwip
{
public:
    lwip(void* context);
    ~lwip();

    /* stack is movable */
    lwip& operator=(lwip&& other) noexcept;
    lwip(lwip&& other) noexcept;

    /* stack is non-copyable */
    lwip(const lwip&) = delete;
    lwip& operator=(const lwip&&) = delete;

    std::string id() const { return "stack0"; } /* only 1 stack for now */
    std::vector<std::string> interface_ids() const;
    std::optional<packetio::interface::generic_interface>
    interface(std::string_view id) const;

    tl::expected<std::string, std::string>
    create_interface(const packetio::interface::config_data& config);
    void delete_interface(std::string_view id);

    std::unordered_map<std::string, stack::stats_data> stats() const;

    using interface_map =
        std::map<std::string, std::unique_ptr<net_interface>, std::less<>>;

private:
    packetio::internal::api::client m_client;
    std::vector<std::string> m_tasks;
    interface_map m_interfaces;
    int m_timerfd;
};

} // namespace openperf::packet::stack::dpdk

#endif /* _OP_PACKETIO_STACK_LWIP_HPP_ */
