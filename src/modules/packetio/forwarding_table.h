#ifndef _ICP_PACKETIO_FORWARDING_TABLE_H_
#define _ICP_PACKETIO_FORWARDING_TABLE_H_

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

#include "net/net_types.h"

namespace icp::packetio {

template <typename Interface, typename Sink, int MaxPorts>
class forwarding_table
{
    using interface_map = immer::map<net::mac_address, Interface*>;
    using sink_vector   = immer::flex_vector<Sink>;

    static constexpr unsigned mac_address_length = 6;

    std::array<std::atomic<interface_map*>, MaxPorts> m_interfaces;
    std::array<std::atomic<sink_vector*>,   MaxPorts> m_sinks;

public:
    forwarding_table();
    ~forwarding_table();

    interface_map* insert_interface(uint16_t port_idx, const net::mac_address& mac,
                                    Interface* ifp);
    interface_map* remove_interface(uint16_t port_idx, const net::mac_address& mac);

    sink_vector* insert_sink(uint16_t port_idx, Sink sink);
    sink_vector* remove_sink(uint16_t port_idx, Sink sink);

    Interface* find_interface(std::string_view id) const;

    Interface* find_interface(uint16_t port_idx, const net::mac_address& mac) const;
    Interface* find_interface(uint16_t port_idx, const uint8_t octets[mac_address_length]) const;

    interface_map& get_interfaces(uint16_t port_idx) const;
    sink_vector&   get_sinks(uint16_t port_idx) const;
};

}

#endif /* _ICP_PACKETIO_FORWARDING_TABLE_H_ */
