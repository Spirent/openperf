#include <cassert>
#include <vector>

#include "tl/expected.hpp"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/port_filter.h"

struct rte_flow;

namespace openperf::packetio::dpdk::port {

enum class flow_error_type {validation, addition, deletion};

enum class rule_action : bool { none = false, add_stack_tag = true };
/*
 * Try to log a useful message given the peculiar nature of the rte_flow_error
 * structure.  Some drivers don't bother filling in some of the fields!
 */
static void log_flow_error(enum flow_error_type type, const rte_flow_error& error)
{
    std::string_view action = (type == flow_error_type::validation ? "validation"
                               : type == flow_error_type::addition ? "addition"
                               : "deletion");
    if (error.message) {
        OP_LOG(OP_LOG_ERROR, "Flow %.*s failed: %s\n",
                static_cast<int>(action.size()), action.data(),
                error.message);
    } else if (error.cause) {
        OP_LOG(OP_LOG_ERROR, "Flow %.*s failed: %s\n",
                static_cast<int>(action.size()), action.data(),
                error.cause);
    } else {
        OP_LOG(OP_LOG_ERROR, "Flow %.*s failed: %d\n",
                static_cast<int>(action.size()), action.data(),
                error.type);
    }
}

static tl::expected<rte_flow*, int> add_ethernet_flow(uint16_t port_id,
                                                      const rte_flow_item_eth* eth_spec,
                                                      const rte_flow_item_eth* eth_mask,
                                                      rule_action action)
{
    /*
     * We want to distribute our flows across all queues via RSS.  To do that,
     * we need to query some RSS information from the port so that we can generate
     * the correct RSS flow action.
     */
    std::vector<uint16_t> rx_queues(model::port_info(port_id).rx_queue_count());
    std::iota(std::begin(rx_queues), std::end(rx_queues), 0);
    const auto action_rss = rte_flow_action_rss{
        .func = RTE_ETH_HASH_FUNCTION_TOEPLITZ,
        .level = 0,
        .types = model::port_info(port_id).rss_offloads(),
        .key_len = 0,
        .queue_num = static_cast<unsigned>(rx_queues.size()),
        .key = nullptr,
        .queue = rx_queues.data(),
    };

    /*
     * Once we have the RSS action, the rest of the configuration is
     * straight forward.
     */
    const auto attr = rte_flow_attr{ .ingress = 1 };

    std::array<rte_flow_item, 2> items = {
        rte_flow_item{
            .type = RTE_FLOW_ITEM_TYPE_ETH,
            .spec = eth_spec,
            .last = nullptr,
            .mask = eth_mask,
        },
        rte_flow_item{
            .type = RTE_FLOW_ITEM_TYPE_END,
        },
    };

    const auto action_mark = rte_flow_action_mark{
        .id = (action == rule_action::add_stack_tag ? 1U : 0U),
    };

    std::array<rte_flow_action, 3> actions = {
        rte_flow_action{
            .type = RTE_FLOW_ACTION_TYPE_MARK,
            .conf = &action_mark,
        },
        rte_flow_action{
            .type = RTE_FLOW_ACTION_TYPE_RSS,
            .conf = &action_rss,
        },
        rte_flow_action{
            .type = RTE_FLOW_ACTION_TYPE_END,
        },
    };

    auto flow_error = rte_flow_error{};

    if (auto error = rte_flow_validate(port_id, &attr, items.data(), actions.data(), &flow_error); error != 0) {
        log_flow_error(flow_error_type::validation, flow_error);
        return (tl::make_unexpected(std::abs(error)));
    }

    auto flow = rte_flow_create(port_id, &attr, items.data(), actions.data(), &flow_error);
    if (!flow) {
        log_flow_error(flow_error_type::addition, flow_error);
        return (tl::make_unexpected(rte_errno));  /* What else could we return? */
    }

    return (flow);
}

static tl::expected<rte_flow*, int> add_ethernet_flow(uint16_t port_id,
                                                      const net::mac_address& mac,
                                                      rule_action action)
{
    auto eth_spec = rte_flow_item_eth{
        .dst.addr_bytes = { mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] },
    };

    auto eth_mask = rte_flow_item_eth{
        .dst.addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    };

    return (add_ethernet_flow(port_id, &eth_spec, &eth_mask, action));
}

static bool delete_flow(uint16_t port_id, struct rte_flow* flow)
{
    auto flow_error = rte_flow_error{};
    if (rte_flow_destroy(port_id, flow, &flow_error) != 0) {
        log_flow_error(flow_error_type::deletion, flow_error);
        return (false);
    }

    return (true);
}

static void maybe_invoke_callback(const filter_event_add& add)
{
    if (add.on_overflow) {
        add.on_overflow.value()();
    }
}

static void maybe_invoke_callback(const filter_event_del& del)
{
    if (del.on_underflow) {
        del.on_underflow.value()();
    }
}

flow_filter::flow_filter(uint16_t port_id)
    : m_port(port_id)
{
    auto broadcast = rte_flow_item_eth{
        .dst.addr_bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    };

    auto flow = add_ethernet_flow(port_id, &broadcast, &broadcast,
                                  rule_action::add_stack_tag);
    if (!flow) {
        throw std::runtime_error("Could not add broadcast flow rule");
    }

    m_flows.emplace(net::mac_address(broadcast.dst.addr_bytes), *flow);

    /* XXX: Add IPv6 multicast rules when IPv6 support is added */
}

flow_filter::~flow_filter()
{
    if (m_flows.empty()) return;

    rte_flow_flush(m_port, nullptr);
}

flow_filter::flow_filter(flow_filter&& other)
    : m_port(other.m_port)
    , m_flows(std::move(other.m_flows))
    , m_overflows(std::move(other.m_overflows))
{}

flow_filter& flow_filter::operator=(flow_filter&& other)
{
    if (this != &other) {
        m_port = other.m_port;
        m_flows = std::move(other.m_flows);
        m_overflows = std::move(other.m_overflows);
    }
    return (*this);
}

uint16_t flow_filter::port_id() const
{
    return (m_port);
}

/* Promiscuous ethernet spec and mask is just a zeroed out structure */
static constexpr rte_flow_item_eth promiscuous = {};

static bool filter_full(uint16_t port_id)
{
    /*
     * Dummy ethernet spec and mask.  This allows receipt of packets
     * from multicast sources.  This really shouldn't do anything useful,
     * but it provides a nice rule to allow us to test if the flow table
     * is full.  The API provides no means to do so.
     */
    static constexpr rte_flow_item_eth dummy = {
        .src.addr_bytes = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 },
    };

    auto flow = add_ethernet_flow(port_id, &dummy, &dummy, rule_action::none);
    if (!flow) { return (true); }  /* assume device is full */

    delete_flow(port_id, *flow);

    return (false);
}

/*
 * Attempt to move a MAC from overflows to flow.
 */
static void maybe_move_overflow_to_filter(uint16_t port_id,
                                          std::unordered_map<net::mac_address, rte_flow*>& flows,
                                          std::vector<net::mac_address>& overflows)
{
    if (overflows.empty()) return;  /* nothing to do */

    if (auto flow = add_ethernet_flow(port_id, overflows.front(),
                                      rule_action::add_stack_tag);
        flow.has_value()) {
        flows.emplace(overflows.front(), *flow);
        std::rotate(std::begin(overflows), std::begin(overflows) + 1,
                    std::end(overflows));
        overflows.pop_back();
    }
}

static bool maybe_disable_promiscuous_mode(uint16_t port_id,
                                           std::unordered_map<net::mac_address, rte_flow*>& flows)
{
    if (auto item = flows.find(net::mac_address(promiscuous.dst.addr_bytes));
        item != flows.end()) {
        if (delete_flow(port_id, item->second)) {
            OP_LOG(OP_LOG_INFO, "Disabling promiscuous mode on port %u\n", port_id);
            flows.erase(item);
            return (true);
        }
    }

    return (false);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_add& add,
                                                  const filter_state_ok&)
{
    /*
     * As far as we know, the table isn't full.  Add the promiscuous rule
     * in case this is the last rule we can add.
     */
    auto promiscuous_flow = add_ethernet_flow(m_port, &promiscuous, &promiscuous,
                                              rule_action::none);
    if (!promiscuous_flow) {
        return (filter_state_error{});
    } else if (filter_full(m_port)) {
        OP_LOG(OP_LOG_INFO, "Enabling promiscuous mode on port %u\n", m_port);
        m_flows.emplace(net::mac_address(promiscuous.dst.addr_bytes), *promiscuous_flow);

        m_overflows.push_back(add.mac);
        maybe_invoke_callback(add);

        return (filter_state_overflow{});
    }

    /* Add the actual rule we want */
    auto flow = add_ethernet_flow(m_port, add.mac, rule_action::add_stack_tag);
    if (!flow) {
        return (filter_state_error{});
    }
    m_flows.emplace(add.mac, *flow);

    /* And destroy the promiscuous rule we don't need */
    if (!delete_flow(m_port, *promiscuous_flow)) {
        /* I guess we keep the pointer? */
        m_flows.emplace(net::mac_address(promiscuous.dst.addr_bytes), *promiscuous_flow);
        return (filter_state_error{});
    }

    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_del& del,
                                                  const filter_state_ok&)
{
    auto item = m_flows.find(del.mac);
    if (item == m_flows.end()) return (std::nullopt);

    if (!delete_flow(m_port, item->second)) {
        return (filter_state_error{});
    }
    m_flows.erase(del.mac);

    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_disable&, const filter_state_ok&)
{
    auto promiscuous_flow = add_ethernet_flow(m_port, &promiscuous, &promiscuous,
                                              rule_action::none);
    if (!promiscuous_flow) {
        return (filter_state_error{});
    }

    OP_LOG(OP_LOG_INFO, "Enabling promiscuous mode on port %u\n", m_port);
    m_flows.emplace(net::mac_address(promiscuous.dst.addr_bytes), *promiscuous_flow);

    return (filter_state_disabled{});
}

std::optional<filter_state> flow_filter::on_event(const filter_event_add& add, const filter_state_overflow&)
{
    m_overflows.push_back(add.mac);
    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_del& del, const filter_state_overflow&)
{
    /* Check if this MAC is in our flow table */
    if (auto item = m_flows.find(del.mac); item != m_flows.end()) {
        if (!delete_flow(m_port, item->second)) {
            return (filter_state_error{});
        }
        m_flows.erase(item);
        maybe_move_overflow_to_filter(m_port, m_flows, m_overflows);
    } else {
        /* Item must be in overflow table */
        m_overflows.erase(std::remove(std::begin(m_overflows), std::end(m_overflows), del.mac),
                          std::end(m_overflows));
     }

    if (m_overflows.empty()) {
        /* Delete the promiscuous rule; it's not needed */
        if (!maybe_disable_promiscuous_mode(m_port, m_flows)) {
            return (filter_state_error{});
        }
        maybe_invoke_callback(del);
        return (filter_state_ok{});
    }

    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_add& add, const filter_state_disabled&)
{
    /* Try to add the rule to our flow table */
    auto flow = add_ethernet_flow(m_port, add.mac, rule_action::add_stack_tag);
    if (!flow) {
        /* Filter must be full, add to overflow */
        bool empty = m_overflows.empty();
        m_overflows.push_back(add.mac);

        /* And invoke the callback if this is the first overflowed entry */
        if (empty) maybe_invoke_callback(add);
    } else {
        m_flows.emplace(add.mac, *flow);
    }

    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_del& del, const filter_state_disabled&)
{
    if (auto item = m_flows.find(del.mac); item != m_flows.end()) {
        if (!delete_flow(m_port, item->second)) {
            return (filter_state_error{});
        }
        m_flows.erase(item);
        maybe_move_overflow_to_filter(m_port, m_flows, m_overflows);
    } else {
        /* Item must be in overflow table */
        m_overflows.erase(std::remove(std::begin(m_overflows), std::end(m_overflows), del.mac),
                          std::end(m_overflows));
    }

    if (m_overflows.empty()) maybe_invoke_callback(del);

    return (std::nullopt);
}

std::optional<filter_state> flow_filter::on_event(const filter_event_enable&, const filter_state_disabled&)
{
    if (!maybe_disable_promiscuous_mode(m_port, m_flows)) {
        return (filter_state_error{});
    }
    return (filter_state_ok{});
}

}
