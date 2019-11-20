#ifndef _OP_PACKETIO_DPDK_PORT_FILTER_HPP_
#define _OP_PACKETIO_DPDK_PORT_FILTER_HPP_

#include <functional>
#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>

#include "core/op_log.h"
#include "net/net_types.hpp"
#include "packetio/generic_interface.hpp"
#include "utils/variant_index.hpp"

struct rte_flow;

namespace openperf::packetio::dpdk::port {

/**
 * Filter states
 */
struct filter_state_ok {};        /*<< Filter is enabled and working */
struct filter_state_overflow {};  /*<< Port is promiscuous due to filter overflow */
struct filter_state_disabled {};  /*<< Port is promiscuous due to administrative event */
struct filter_state_error {};     /*<< Filter returned an error; no further modifications are possible */

using filter_state = std::variant<filter_state_ok,
                                  filter_state_overflow,
                                  filter_state_disabled,
                                  filter_state_error>;

/**
 * Filter events
 */
struct filter_event_add {
    net::mac_address mac;
    std::optional<std::function<void()>> on_overflow = std::nullopt;
};

struct filter_event_del {
    net::mac_address mac;
    std::optional<std::function<void()>> on_underflow = std::nullopt;
};

struct filter_event_disable {};
struct filter_event_enable {};

using filter_event = std::variant<filter_event_add,
                                  filter_event_del,
                                  filter_event_disable,
                                  filter_event_enable>;

template <typename Derived, typename StateVariant, typename EventVariant>
class filter_state_machine
{
    StateVariant m_state;

public:
    void handle_event(const EventVariant& event)
    {
        Derived& child = static_cast<Derived&>(*this);
        auto next_state = std::visit(
            [&](const auto& action, const auto& state) {
                return (child.on_event(action, state));
            }, event, m_state);

        if (next_state) {
            OP_LOG(OP_LOG_TRACE, "Port Filter %u: %s --> %s\n",
                    reinterpret_cast<Derived&>(*this).port_id(),
                    to_string(m_state).data(),
                    to_string(*next_state).data());
            m_state = *std::move(next_state);
        }
    }
};

class flow_filter : public filter_state_machine<flow_filter, filter_state, filter_event>
{
    uint16_t m_port;

    std::unordered_map<net::mac_address, rte_flow*> m_flows;
    std::vector<net::mac_address> m_overflows;

public:
    flow_filter(uint16_t port_id);
    ~flow_filter();

    flow_filter(flow_filter&&);
    flow_filter& operator=(flow_filter&&);

    uint16_t port_id() const;

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_disable&, const filter_state_ok&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_overflow&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_overflow&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_enable&, const filter_state_disabled&);

    /* Default action for any unhandled cases */
    template <typename Event, typename State>
    std::optional<filter_state> on_event(const Event&, const State&)
    {
        return (std::nullopt);
    }
};

class mac_filter : public filter_state_machine<mac_filter, filter_state, filter_event>
{
    uint16_t m_port;
    std::vector<net::mac_address> m_filtered;
    std::vector<net::mac_address> m_overflowed;

public:
    mac_filter(uint16_t port_id);
    ~mac_filter();

    mac_filter(mac_filter&&);
    mac_filter& operator=(mac_filter&&);

    uint16_t port_id() const;

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_ok&);
    std::optional<filter_state> on_event(const filter_event_disable&, const filter_state_ok&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_overflow&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_overflow&);

    std::optional<filter_state> on_event(const filter_event_add& add, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_del& del, const filter_state_disabled&);
    std::optional<filter_state> on_event(const filter_event_enable&, const filter_state_disabled&);

    /* Default action for any unhandled cases */
    template <typename Event, typename State>
    std::optional<filter_state> on_event(const Event&, const State&)
    {
        return (std::nullopt);
    }
};

using filter_variant = std::variant<flow_filter, mac_filter>;

enum class filter_type {
    flow = utils::variant_index<filter_variant, flow_filter>(),
    mac  = utils::variant_index<filter_variant, mac_filter>(),
};

class filter
{
public:
    filter(uint16_t port_id);

    uint16_t port_id() const;
    filter_type type() const;

    void add_mac_address(const net::mac_address& mac);
    void add_mac_address(const net::mac_address& mac, std::function<void()>&& on_overflow);

    void del_mac_address(const net::mac_address& mac);
    void del_mac_address(const net::mac_address& mac, std::function<void()>&& on_underflow);

    void enable();
    void disable();

    using filter_strategy = std::variant<flow_filter, mac_filter>;

private:
    filter_strategy m_filter;
};

const char * to_string(const filter_state&);

}

#endif /* _OP_PACKETIO_DPDK_PORT_FILTER_HPP_ */
