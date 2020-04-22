#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"

namespace openperf::packet::generator::traffic::protocol {

namespace ethernet {

inline constexpr uint16_t ether_type_ipv4 = 0x0800;
inline constexpr uint16_t ether_type_ipv6 = 0x86dd;
inline constexpr uint16_t ether_type_arp = 0x0806;
inline constexpr uint16_t ether_type_vlan = 0x8100;
inline constexpr uint16_t ether_type_qinq = 0x88a8;
inline constexpr uint16_t ether_type_mpls = 0x8847;
inline constexpr uint16_t ether_type_multicast_mpls = 0x8848;

} // namespace ethernet

void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::ipv4&);
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::ipv6&);
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::vlan&);
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::mpls&);

void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv4&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv6&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::vlan&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::mpls&);

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_ */
