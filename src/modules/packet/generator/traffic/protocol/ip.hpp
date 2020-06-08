#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"
#include "packet/generator/traffic/protocol/custom.hpp"

namespace openperf::packet::generator::traffic::protocol {

namespace ip {

inline constexpr uint8_t icmp = 1;
inline constexpr uint8_t igmp = 2;
inline constexpr uint8_t ip_in_ip = 4;
inline constexpr uint8_t tcp = 6;
inline constexpr uint8_t udp = 17;
inline constexpr uint8_t ipv6 = 41;
inline constexpr uint8_t ipv6_route = 43;
inline constexpr uint8_t ipv6_frag = 44;
inline constexpr uint8_t ipv6_icmp = 58;
inline constexpr uint8_t ipv6_none = 59;
inline constexpr uint8_t ipv6_opts = 60;
inline constexpr uint8_t etherip = 97;
inline constexpr uint8_t encap = 98;
inline constexpr uint8_t l2tp = 115;
inline constexpr uint8_t sctp = 132;
inline constexpr uint8_t mpls_in_ip = 137;

} // namespace ip

void update_context(libpacket::protocol::ipv4&, const custom&) noexcept;
void update_context(libpacket::protocol::ipv4&,
                    const libpacket::protocol::tcp&) noexcept;
void update_context(libpacket::protocol::ipv4&,
                    const libpacket::protocol::udp&) noexcept;

void update_context(libpacket::protocol::ipv6&, const custom&) noexcept;
void update_context(libpacket::protocol::ipv6&,
                    const libpacket::protocol::tcp&) noexcept;
void update_context(libpacket::protocol::ipv6&,
                    const libpacket::protocol::udp&) noexcept;

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags,
                         const libpacket::protocol::ipv4&) noexcept;
flags update_packet_type(flags flags,
                         const libpacket::protocol::ipv6&) noexcept;

using header_lengths = packetio::packet::header_lengths;
header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::ipv4&) noexcept;
header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::ipv6&) noexcept;

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_ */
