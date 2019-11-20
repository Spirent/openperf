#include <algorithm>
#include <cassert>

#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/drivers/dpdk/port_filter.h"

namespace openperf::packetio::dpdk::port {

static struct rte_ether_addr* to_dpdk_mac(const net::mac_address& mac)
{
    return (reinterpret_cast<rte_ether_addr*>(const_cast<uint8_t*>(mac.data())));
}

static unsigned get_max_mac_addresses(uint16_t port_id)
{
    /*
     * Ports have a "default" address that is included in this count.
     * Hence, we have 1 less than the max usable addresses.
     */
    return (model::port_info(port_id).max_mac_addrs() - 1);
}

static void maybe_enable_promiscuous_mode(uint16_t port_id)
{
    if (!rte_eth_promiscuous_get(port_id)) {
        OP_LOG(OP_LOG_INFO, "Enabling promiscuous mode on port %u\n", port_id);
        rte_eth_promiscuous_enable(port_id);
    }
}

static void maybe_disable_promiscuous_mode(uint16_t port_id)
{
    if (rte_eth_promiscuous_get(port_id)) {
        OP_LOG(OP_LOG_INFO, "Disabling promiscuous mode on port %u\n", port_id);
        rte_eth_promiscuous_disable(port_id);
    }
}

mac_filter::mac_filter(uint16_t port_id)
    : m_port(port_id)
{}

mac_filter::~mac_filter()
{
    if (m_filtered.empty()) return;

    std::for_each(std::begin(m_filtered), std::end(m_filtered),
                  [&](const auto& mac) {
                      rte_eth_dev_mac_addr_remove(m_port, to_dpdk_mac(mac));
                  });

    maybe_disable_promiscuous_mode(m_port);
}

mac_filter::mac_filter(mac_filter&& other)
    : m_port(other.m_port)
    , m_filtered(std::move(other.m_filtered))
    , m_overflowed(std::move(other.m_overflowed))
{}

mac_filter& mac_filter::operator=(mac_filter&& other)
{
    if (this != &other) {
        m_port = other.m_port;
        m_filtered = std::move(other.m_filtered);
        m_overflowed = std::move(other.m_overflowed);
    }
    return (*this);
}

uint16_t mac_filter::port_id() const
{
    return (m_port);
}

static filter_state_error on_error(const filter_event_add& add, uint16_t port_id, int error)
{
    OP_LOG(OP_LOG_ERROR, "Failed to add address %s to port %u: %s\n",
            net::to_string(add.mac).c_str(), port_id, strerror(std::abs(error)));
    maybe_enable_promiscuous_mode(port_id);
    return (filter_state_error{});
}

static filter_state_error on_error(const filter_event_del& del, uint16_t port_id, int error)
{
    OP_LOG(OP_LOG_ERROR, "Failed to remove address %s from port %u: %s\n",
            net::to_string(del.mac).c_str(), port_id, strerror(std::abs(error)));
    return (filter_state_error{});
}

static std::optional<filter_state_error> maybe_delete_mac(uint16_t port_id,
                                                          const net::mac_address& mac,
                                                          std::vector<net::mac_address>& filtered,
                                                          std::vector<net::mac_address>& overflowed)
{
    /* Find the MAC in the appropriate list and delete it */
    if (auto item = std::find(std::begin(filtered), std::end(filtered), mac);
        item != std::end(filtered)) {
        if (auto error = rte_eth_dev_mac_addr_remove(port_id, to_dpdk_mac(mac)); error != 0) {
            return (on_error(filter_event_del{ mac }, port_id, error));
        }
        filtered.erase(std::remove(std::begin(filtered), std::end(filtered), mac),
                       std::end(filtered));
    }

    if (auto item = std::find(std::begin(overflowed), std::end(overflowed), mac);
               item != std::end(overflowed)) {
        overflowed.erase(std::remove(std::begin(overflowed), std::end(overflowed), mac),
                         std::end(overflowed));
    }

    /* Note: the MAC might not be in either list; that's ok */

    /* Check if it's safe to move an overflowed MAC to the filter */
    if (overflowed.size() && filtered.size() < get_max_mac_addresses(port_id)) {
        /* Remove a MAC from the overflow list and add it to the filtered list */
        if (auto error = rte_eth_dev_mac_addr_add(port_id, to_dpdk_mac(overflowed.front()), 0); error != 0) {
            return (on_error(filter_event_add{ overflowed.front() }, port_id, error));
        }

        /* Drop the added MAC from the overflow list */
        filtered.push_back(overflowed.front());
        std::rotate(std::begin(overflowed), std::begin(overflowed) + 1,
                    std::end(overflowed));
        overflowed.pop_back();
    }

    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_add& add, const filter_state_ok&)
{
    if (m_filtered.size() == get_max_mac_addresses(m_port)) {
        m_overflowed.push_back(add.mac);
        maybe_enable_promiscuous_mode(m_port);
        return (filter_state_overflow{});
    }

    if (auto error = rte_eth_dev_mac_addr_add(m_port, to_dpdk_mac(add.mac), 0); error != 0) {
        return (on_error(add, m_port, error));
    }

    m_filtered.push_back(add.mac);

    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_del& del, const filter_state_ok&)
{
    if (auto error = rte_eth_dev_mac_addr_remove(m_port, to_dpdk_mac(del.mac)); error != 0) {
        return (on_error(del, m_port, error));
    }

    m_filtered.erase(std::remove(std::begin(m_filtered), std::end(m_filtered), del.mac),
                     std::end(m_filtered));

    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_disable&, const filter_state_ok&)
{
    maybe_enable_promiscuous_mode(m_port);
    return (filter_state_disabled{});
}

std::optional<filter_state> mac_filter::on_event(const filter_event_add& add, const filter_state_overflow&)
{
    assert(!m_overflowed.empty());

    m_overflowed.push_back(add.mac);
    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_del& del, const filter_state_overflow&)
{
    assert(!m_overflowed.empty());

    if (auto error = maybe_delete_mac(m_port, del.mac, m_filtered, m_overflowed);
        error.has_value()) {
        return (filter_state_error{});
    }

    /* Check if we can disable promiscuous mode */
    if (m_overflowed.empty()) {
        maybe_disable_promiscuous_mode(m_port);
        return (filter_state_ok{});
    }

    /* Nope; must still be overflowing */
    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_add& add, const filter_state_disabled&)
{
    /* See if we have room for the address in our filter */
    if (m_filtered.size() < get_max_mac_addresses(m_port)) {
        if (auto error = rte_eth_dev_mac_addr_add(m_port, to_dpdk_mac(add.mac), 0); error != 0) {
            return (on_error(add, m_port, error));
        }
        return (std::nullopt);
    }

    /* No room; add new mac to the overflow list */
    m_overflowed.push_back(add.mac);
    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_del& del, const filter_state_disabled&)
{
    if (auto error = maybe_delete_mac(m_port, del.mac, m_filtered, m_overflowed);
        error.has_value()) {
        return (filter_state_error{});
    }

    return (std::nullopt);
}

std::optional<filter_state> mac_filter::on_event(const filter_event_enable&, const filter_state_disabled&)
{
    if (!m_overflowed.empty()) return (filter_state_overflow{});

    maybe_disable_promiscuous_mode(m_port);
    return (filter_state_ok{});
}

}
