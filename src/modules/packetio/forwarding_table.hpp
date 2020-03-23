#ifndef _OP_PACKETIO_FORWARDING_TABLE_HPP_
#define _OP_PACKETIO_FORWARDING_TABLE_HPP_

/**
 * @file
 *
 * The forwarding_table contains all of the details needed to dispatch
 * packets on the receive path.
 */

#include <array>
#include <atomic>

#include "immer/flex_vector.hpp"
#include "immer/map.hpp"

#include "net/net_types.hpp"

namespace openperf::packetio {

template <typename Interface, typename Sink, int MaxPorts>
class forwarding_table
{
    struct interface_sinks
    {
        Interface* ifp;
        std::vector<Sink> sinks;
    };

    using interface_map = immer::map<net::mac_address, interface_sinks>;
    using sink_vector = immer::flex_vector<Sink>;

    static constexpr unsigned mac_address_length = 6;

    std::array<std::atomic<interface_map*>, MaxPorts> m_interfaces;
    std::array<std::atomic<sink_vector*>, MaxPorts> m_port_sinks;
    std::array<std::atomic<int>, MaxPorts> m_port_interface_sink_count;

public:
    forwarding_table();
    ~forwarding_table();

    interface_map* insert_interface(uint16_t port_idx,
                                    const net::mac_address& mac,
                                    Interface* ifp);
    interface_map* remove_interface(uint16_t port_idx,
                                    const net::mac_address& mac);

    sink_vector* insert_sink(uint16_t port_idx, Sink sink);
    sink_vector* remove_sink(uint16_t port_idx, Sink sink);

    interface_map* insert_interface_sink(uint16_t port_idx,
                                         const net::mac_address& mac,
                                         Interface* ifp,
                                         Sink sink);
    interface_map* remove_interface_sink(uint16_t port_idx,
                                         const net::mac_address& mac,
                                         Interface* ifp,
                                         Sink sink);

    Interface* find_interface(uint16_t port_idx, std::string_view id) const;
    Interface* find_interface(std::string_view id) const;

    Interface* find_interface(uint16_t port_idx,
                              const net::mac_address& mac) const;
    Interface* find_interface(uint16_t port_idx,
                              const uint8_t octets[mac_address_length]) const;

    interface_map& get_interfaces(uint16_t port_idx) const;

    sink_vector& get_sinks(uint16_t port_idx) const;

    bool has_interface_sinks(uint16_t port_idx) const;
    const std::vector<Sink>*
    find_interface_sinks(uint16_t port_idx, const net::mac_address& mac) const;
    const interface_sinks*
    find_interface_and_sinks(uint16_t port_idx,
                             const net::mac_address& mac) const;
    /**
     * Visit all the interfaces sinks.
     *
     * This function is not intended for use in the fast path packet dispatch.
     * Due to std::function performance overhead it is better to use
     * get_interfaces().
     */
    void visit_interface_sinks(
        uint16_t port_idx,
        std::function<bool(Interface* ifp, const Sink& sink)>&& visitor) const;
};

} // namespace openperf::packetio

#endif /* _OP_PACKETIO_FORWARDING_TABLE_HPP_ */
