#include <cassert>
#include <optional>

#include "packetio/forwarding_table.h"

namespace icp::packetio {

/**
 * A note about our keys:
 * We use the 16-bit port index and the 48-bit MAC address to generate
 * a 64 bit key of the form [port_idx | MAC].
 * This does two things for us:
 * 1. Packets received on the wrong port won't be forwarded to a matching
 *    interface.
 * 2. We can support the same MAC address on multiple ports.
 *
 * As we expand the L2 protocols that we support, we will need to expand
 * the size of this key.
 **/
static uint64_t to_key(uint16_t port_idx, const net::mac_address& mac)
{
    return (static_cast<uint64_t>(port_idx) << 48
            | static_cast<uint64_t>(mac[0]) << 40
            | static_cast<uint64_t>(mac[1]) << 32
            | mac[2] << 24
            | mac[3] << 16
            | mac[4] << 8
            | mac[5]);
}

static uint64_t to_key(uint16_t port_idx, const uint8_t octets[6])
{
    return (static_cast<uint64_t>(port_idx) << 48
            | static_cast<uint64_t>(octets[0]) << 40
            | static_cast<uint64_t>(octets[1]) << 32
            | octets[2] << 24
            | octets[3] << 16
            | octets[4] << 8
            | octets[5]);
}

template <typename Vector, typename Item>
std::optional<typename Vector::size_type> find(typename Vector::const_iterator first,
                                               typename Vector::const_iterator last,
                                               Item& value)
{
    typename Vector::size_type idx = 0;
    for(; first != last; ++first, ++idx) {
        if (*first == value) return (idx);
    }
    return (std::nullopt);
}

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
    auto updated = new interface_map(original->set(to_key(port_idx, mac), ifp));
    return (m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::interface_map*
forwarding_table<Interface, Sink, MaxPorts>::remove_interface(
    uint16_t port_idx, const net::mac_address& mac)
{
    assert(port_idx < MaxPorts);

    auto original = m_interfaces[port_idx].load(std::memory_order_relaxed);
    auto updated = new interface_map(original->erase(to_key(port_idx, mac)));
    return (m_interfaces[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::insert_sink(uint16_t port_idx, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_sinks[port_idx].load(std::memory_order_relaxed);
    auto updated = new sink_vector(original->push_back(sink));
    return (m_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
typename forwarding_table<Interface, Sink, MaxPorts>::sink_vector*
forwarding_table<Interface, Sink, MaxPorts>::remove_sink(uint16_t port_idx, Sink sink)
{
    assert(port_idx < MaxPorts);

    auto original = m_sinks[port_idx].load(std::memory_order_relaxed);
    auto idx = find<sink_vector>(original->begin(), original->end(), sink);
    if (!idx) return (nullptr);
    auto updated = new sink_vector(original->erase(*idx));
    return (m_sinks[port_idx].exchange(updated, std::memory_order_release));
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const net::mac_address& mac) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(to_key(port_idx, mac));
    return (item ? *item : nullptr);
}

template <typename Interface, typename Sink, int MaxPorts>
Interface* forwarding_table<Interface, Sink, MaxPorts>::find_interface(
    uint16_t port_idx, const uint8_t octets[mac_address_length]) const
{
    assert(port_idx < MaxPorts);

    auto map = m_interfaces[port_idx].load(std::memory_order_consume);
    auto item = map->find(to_key(port_idx, octets));
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
