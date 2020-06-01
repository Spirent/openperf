#include "packet/type/mac_address.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/port_filter.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::dpdk::port {

using mac_address = libpacket::type::mac_address;

std::string_view to_string(filter_state& state)
{
    return (std::visit(utils::overloaded_visitor(
                           [](filter_state_ok&) { return "ok"; },
                           [](filter_state_overflow&) { return "overflow"; },
                           [](filter_state_disabled&) { return "disabled"; },
                           [](filter_state_error&) { return "error"; }),
                       state));
}

static filter::filter_strategy make_filter(uint16_t port_id)
{
    auto flow_error = rte_flow_error{};
    if (rte_flow_isolate(port_id, 1, &flow_error) == 0) {
        OP_LOG(OP_LOG_DEBUG, "Using flow filter for port %u\n", port_id);
        return (flow_filter(port_id));
    }

    OP_LOG(OP_LOG_DEBUG, "Using mac filter for port %u\n", port_id);
    return (mac_filter(port_id));
}

filter::filter(uint16_t port_id)
    : m_filter(make_filter(port_id))
{}

uint16_t filter::port_id() const
{
    auto id_visitor = [](auto& filter) { return filter.port_id(); };
    return (std::visit(id_visitor, m_filter));
}

filter_type filter::type() const
{
    return (static_cast<filter_type>(m_filter.index()));
}

void filter::add_mac_address(const mac_address& mac)
{
    auto add_filter_visitor = [&](auto& filter) {
        auto event = filter_event_add{mac};
        filter.handle_event(event);
    };
    std::visit(add_filter_visitor, m_filter);
}

void filter::add_mac_address(const mac_address& mac,
                             std::function<void()>&& on_overflow)
{
    auto add_filter_visitor = [&](auto& filter) {
        auto event = filter_event_add{
            mac, std::forward<std::function<void()>>(on_overflow)};
        filter.handle_event(event);
    };
    std::visit(add_filter_visitor, m_filter);
}

void filter::del_mac_address(const mac_address& mac)
{
    auto del_filter_visitor = [&](auto& filter) {
        auto event = filter_event_del{mac};
        filter.handle_event(event);
    };
    std::visit(del_filter_visitor, m_filter);
}

void filter::del_mac_address(const mac_address& mac,
                             std::function<void()>&& on_underflow)
{
    auto del_filter_visitor = [&](auto& filter) {
        auto event = filter_event_del{
            mac, std::forward<std::function<void()>>(on_underflow)};
        filter.handle_event(event);
    };
    std::visit(del_filter_visitor, m_filter);
}

void filter::enable()
{
    auto enable_visitor = [](auto& filter) {
        filter.handle_event(filter_event_enable{});
    };
    std::visit(enable_visitor, m_filter);
}

void filter::disable()
{
    auto disable_visitor = [](auto& filter) {
        filter.handle_event(filter_event_disable{});
    };
    std::visit(disable_visitor, m_filter);
}

} // namespace openperf::packetio::dpdk::port
