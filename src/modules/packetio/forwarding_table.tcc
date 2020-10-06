#include <cassert>
#include <string>

#include "packetio/forwarding_table.hpp"

namespace openperf::packetio {

using mac_address = libpacket::type::mac_address;

template <typename Interface>
std::string get_interface_id(const Interface& ifp);

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        m_interfaces[i].store(new interface_map());
        m_port_rx_sinks[i].store(new sink_vector());
        m_port_tx_sinks[i].store(new sink_vector());
        m_port_interface_rx_sink_count[i].store(0);
        m_port_interface_tx_sink_count[i].store(0);
    }
}

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::~forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        delete m_interfaces[i].exchange(nullptr);
        delete m_port_rx_sinks[i].exchange(nullptr);
        delete m_port_tx_sinks[i].exchange(nullptr);
    }
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::insert_interface(
    uint16_t port_idx, const mac_address& mac, Interface interface)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(std::move(
        original->set(mac, interface_sinks{std::move(interface), {}, {}})));
    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface(
    uint16_t port_idx, const mac_address& mac)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(std::move(original->erase(mac)));
    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::insert_sink(uint16_t port_idx,
                                                         direction dir,
                                                         Sink sink)
{
    assert(port_idx < MaxPorts);

    auto& port_sinks = (dir == direction::RX) ? m_port_rx_sinks[port_idx]
                                              : m_port_tx_sinks[port_idx];

    auto original = port_sinks.load(std::memory_order_relaxed);
    auto updated = new sink_vector(std::move(original->push_back(sink)));
    return (port_sinks.exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::remove_sink(uint16_t port_idx,
                                                         direction dir,
                                                         Sink sink)
{
    assert(port_idx < MaxPorts);

    auto& port_sinks = (dir == direction::RX) ? m_port_rx_sinks[port_idx]
                                              : m_port_tx_sinks[port_idx];

    auto original = port_sinks.load(std::memory_order_relaxed);
    auto found = std::find(original->begin(), original->end(), sink);
    if (found == original->end()) return (nullptr); /* not found */
    auto updated = new sink_vector(
        std::move(original->erase(std::distance(original->begin(), found))));
    return (port_sinks.exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::insert_interface_sink(
    uint16_t port_idx, const mac_address& mac, direction dir, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto* entry = original->find(mac);
    if (!entry) { return (nullptr); }

    auto copy = entry->update([&](auto if_sinks) {
        if (dir == direction::RX) {
            if_sinks.rx_sinks.push_back(std::move(sink));
        } else {
            if_sinks.tx_sinks.push_back(std::move(sink));
        }
        return (if_sinks);
    });

    auto updated =
        new interface_map(std::move(original->set(mac, std::move(copy))));

    if (dir == direction::RX) {
        m_port_interface_rx_sink_count[port_idx]++;
    } else {
        m_port_interface_tx_sink_count[port_idx]++;
    }

    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface_sink(
    uint16_t port_idx, const mac_address& mac, direction dir, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto* entry = original->find(mac);
    if (!entry) return nullptr;

    auto copy = entry->update([&](auto if_sinks) {
        auto& sinks =
            dir == direction::RX ? if_sinks.rx_sinks : if_sinks.tx_sinks;
        sinks.erase(std::remove(std::begin(sinks), std::end(sinks), sink),
                    std::end(sinks));
        return (if_sinks);
    });

    auto updated =
        new interface_map(std::move(original->set(mac, std::move(copy))));

    if (dir == direction::RX) {
        m_port_interface_rx_sink_count[port_idx]--;
    } else {
        m_port_interface_tx_sink_count[port_idx]--;
    }

    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
const Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, std::string_view id) const
{
    assert(port_idx < MaxPorts);

    const auto& map = *(m_interfaces[port_idx].load(std::memory_order_consume));
    auto item =
        std::find_if(std::begin(map), std::end(map), [&](const auto& pair) {
            return (get_interface_id(pair.second->ifp) == id);
        });
    if (item != std::end(map)) return (std::addressof(item->second->ifp));

    return (nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
const Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    std::string_view id) const
{
    for (auto& map_ptr : m_interfaces) {
        const auto& map = *(map_ptr.load(std::memory_order_consume));
        auto item =
            std::find_if(std::begin(map), std::end(map), [&](const auto& pair) {
                return (get_interface_id(pair.second->ifp) == id);
            });
        if (item != std::end(map)) return (std::addressof(item->second->ifp));
    }

    return (nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
const Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? std::addressof((*item)->ifp) : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
const Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const uint8_t octets[mac_address_length]) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac_address(octets));
    return (item ? std::addressof((*item)->ifp) : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map&
forwarding_table<Interface, Sink, MaxPorts>::get_interfaces(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_interfaces[port_idx].load(std::memory_order_consume));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector&
forwarding_table<Interface, Sink, MaxPorts>::get_rx_sinks(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_port_rx_sinks[port_idx].load(std::memory_order_consume));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector&
forwarding_table<Interface, Sink, MaxPorts>::get_tx_sinks(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_port_tx_sinks[port_idx].load(std::memory_order_consume));
}

template <typename Interface, typename Sink, int MaxPorts>
bool forwarding_table<Interface, Sink, MaxPorts>::has_interface_rx_sinks(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    auto count = m_port_interface_rx_sink_count[port_idx].load(
        std::memory_order_consume);
    return (count > 0);
}

template <typename Interface, typename Sink, int MaxPorts>
bool forwarding_table<Interface, Sink, MaxPorts>::has_interface_tx_sinks(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    auto count = m_port_interface_tx_sink_count[port_idx].load(
        std::memory_order_consume);
    return (count > 0);
}

template <typename Interface, typename Sink, int MaxPorts>
const std::vector<Sink>*
forwarding_table<Interface, Sink, MaxPorts>::find_interface_rx_sinks(
    uint16_t port_idx, const mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? &((*item)->rx_sinks) : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
const std::vector<Sink>*
forwarding_table<Interface, Sink, MaxPorts>::find_interface_tx_sinks(
    uint16_t port_idx, const mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? &((*item)->tx_sinks) : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
const typename forwarding_table<Interface, Sink, MaxPorts>::interface_sinks*
forwarding_table<Interface, Sink, MaxPorts>::find_interface_and_sinks(
    uint16_t port_idx, const mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? &(item->get()) : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
void forwarding_table<Interface, Sink, MaxPorts>::visit_interface_sinks(
    uint16_t port_idx,
    direction dir,
    std::function<bool(const Interface& ifp, const Sink& sink)>&& visitor) const
{
    assert(port_idx < MaxPorts);

    auto interface_map = m_interfaces[port_idx].load(std::memory_order_consume);
    for (auto [mac, entry] : *interface_map) {
        auto& sinks =
            (dir == direction::RX) ? entry->rx_sinks : entry->tx_sinks;
        for (auto& sink : sinks) {
            if (!visitor(entry->ifp, sink)) return;
        }
    }
}

} // namespace openperf::packetio
