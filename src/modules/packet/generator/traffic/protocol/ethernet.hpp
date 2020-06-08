#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/generator/traffic/protocol/custom.hpp"

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
                    const libpacket::protocol::ipv4&) noexcept;
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::ipv6&) noexcept;
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::vlan&) noexcept;
void update_context(libpacket::protocol::ethernet&,
                    const libpacket::protocol::mpls&) noexcept;
void update_context(libpacket::protocol::ethernet&, const custom&) noexcept;

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags,
                         const libpacket::protocol::ethernet&) noexcept;

using header_lengths = packetio::packet::header_lengths;
header_lengths
update_header_lengths(header_lengths& lengths,
                      const libpacket::protocol::ethernet&) noexcept;

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_ETHERNET_HPP_ */
