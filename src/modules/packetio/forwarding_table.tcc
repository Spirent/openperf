#include <cassert>

#include "packetio/forwarding_table.h"

namespace icp::packetio {

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        m_interfaces[i].store(new interface_map());
        m_sinks[i].store(new sink_vector());
    }
}

template <typename Interface, typename Sink, int MaxPorts>
forwarding_table<Interface, Sink, MaxPorts>::~forwarding_table()
{
    for (int i = 0; i < MaxPorts; i++) {
        delete m_interfaces[i].exchange(nullptr);
        delete m_sinks[i].exchange(nullptr);
    }
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::insert_interface(
    uint16_t port_idx, const net::mac_address& mac, Interface* ifp)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(std::move(original->set(mac, ifp)));
    return (m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface(
    uint16_t port_idx, const net::mac_address& mac)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(std::move(original->erase(mac)));
    return (m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::insert_sink(uint16_t port_idx, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_sinks[port_idx].load(std::memory_order_relaxed);
    auto updated = new sink_vector(std::move(original->push_back(sink)));
    return (m_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::remove_sink(uint16_t port_idx, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_sinks[port_idx].load(std::memory_order_relaxed);
    auto found = std::find(original->begin(), original->end(), sink);
    if (found == original->end()) return (nullptr);  /* not found */
    auto updated = new sink_vector(
        std::move(original->erase(std::distance(original->begin(), found))));
    return (m_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const net::mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(mac);
    return (item ? *item : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const uint8_t octets[mac_address_length]) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(net::mac_address(octets));
    return (item ? *item : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map&
forwarding_table<Interface, Sink, MaxPorts>::get_interfaces(uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_interfaces[port_idx].load(std::memory_order_consume));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector&
forwarding_table<Interface, Sink, MaxPorts>::get_sinks(uint16_t port_idx) const
{
    assert(port_idx < MaxPorts);
    return (*m_sinks[port_idx].load(std::memory_order_consume));
}

}
