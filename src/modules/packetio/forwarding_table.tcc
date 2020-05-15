#include <cassert>
#include <string>

#include "packetio/forwarding_table.hpp"

namespace openperf::packetio {

template <typename Interface> std::string get_interface_id(Interface* ifp);

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        m_interfaces[i].store(new interface_map());
        m_port_sinks[i].store(new sink_vector());
        m_port_interface_sink_count[i].store(0);
    }
}

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::~forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        delete m_interfaces[i].exchange(nullptr);
        delete m_port_sinks[i].exchange(nullptr);
    }
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::insert_interface(
    uint16_t port_idx, const net::mac_address& mac, Interface* ifp)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(std::move(original->set(mac, {ifp, {}})));
    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface(
    uint16_t port_idx, const net::mac_address& mac)
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
                                                         Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_port_sinks[port_idx].load(std::memory_order_relaxed);
    auto updated = new sink_vector(std::move(original->push_back(sink)));
    return (
        m_port_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::remove_sink(uint16_t port_idx,
                                                         Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_port_sinks[port_idx].load(std::memory_order_relaxed);
    auto found = std::find(original->begin(), original->end(), sink);
    if (found == original->end()) return (nullptr); /* not found */
    auto updated = new sink_vector(
        std::move(original->erase(std::distance(original->begin(), found))));
    return (
        m_port_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::insert_interface_sink(
    uint16_t port_idx, const net::mac_address& mac, Interface* ifp, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto entry = (*original)[mac];
    entry.sinks.push_back(sink);
    m_port_interface_sink_count[port_idx]++;
    auto updated = new interface_map(std::move(original->set(mac, entry)));
    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface_sink(
    uint16_t port_idx, const net::mac_address& mac, Interface* ifp, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto entry_ptr = original->find(mac);
    if (!entry_ptr) return nullptr;
    auto entry = *entry_ptr;
    auto& sinks = entry.sinks;

    auto found = std::find(sinks.begin(), sinks.end(), sink);
    if (found == sinks.end()) return (nullptr); /* not found */

    m_port_interface_sink_count[port_idx]--;
    sinks.erase(found);
    auto updated = new interface_map(std::move(original->set(mac, entry)));
    return (
        m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, std::string_view id) const
{
    assert(port_idx < MaxPorts);

    const auto& map = *(m_interfaces[port_idx].load(std::memory_order_consume));
    auto item =
        std::find_if(std::begin(map), std::end(map), [&](const auto& pair) {
            return (get_interface_id(pair.second.ifp) == id);
        });
    if (item != std::end(map)) return (item->second.ifp);

    return (nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    std::string_view id) const
{
    for (auto& map_ptr : m_interfaces) {
        const auto& map = *(map_ptr.load(std::memory_order_consume));
        auto item =
            std::find_if(std::begin(map), std::end(map), [&](const auto& pair) {
                return (get_interface_id(pair.second.ifp) == id);
            });
        if (item != std::end(map)) return (item->second.ifp);
    }

    return (nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const net::mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? item->ifp : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const uint8_t octets[mac_address_length]) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(net::mac_address(octets));
    return (item ? item->ifp : nullptr);
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
forwarding_table<Interface, Sink, MaxPorts>::get_sinks(uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_port_sinks[port_idx].load(std::memory_order_consume));
}

template <typename Interface, typename Sink, int MaxPorts>
bool forwarding_table<Interface, Sink, MaxPorts>::has_interface_sinks(
    uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    auto count =
        m_port_interface_sink_count[port_idx].load(std::memory_order_consume);
    return (count > 0);
}

template <typename Interface, typename Sink, int MaxPorts>
const std::vector<Sink>*
forwarding_table<Interface, Sink, MaxPorts>::find_interface_sinks(
    uint16_t port_idx, const net::mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    if (!item) return nullptr;
    return &item->sinks;
}

template <typename Interface, typename Sink, int MaxPorts>
const typename forwarding_table<Interface, Sink, MaxPorts>::interface_sinks*
forwarding_table<Interface, Sink, MaxPorts>::find_interface_and_sinks(
    uint16_t port_idx, const net::mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return item;
}

template <typename Interface, typename Sink, int MaxPorts>
void forwarding_table<Interface, Sink, MaxPorts>::visit_interface_sinks(
    uint16_t port_idx,
    std::function<bool(Interface* ifp, const Sink& sink)>&& visitor) const
{
    assert(port_idx < MaxPorts);

    auto interface_map = m_interfaces[port_idx].load(std::memory_order_consume);
    for (auto [mac, entry] : *interface_map) {
        for (auto& sink : entry.sinks) {
            if (!visitor(entry.ifp, sink)) return;
        }
    }
}

} // namespace openperf::packetio
