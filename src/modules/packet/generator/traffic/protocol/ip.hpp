#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::protocol {

namespace ip {

inline constexpr uint8_t icmp = 1;
inline constexpr uint8_t igmp = 2;
inline constexpr uint8_t ip_in_ip = 4;
inline constexpr uint8_t tcp = 7;
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

void update_context(libpacket::protocol::ipv4&,
                    const libpacket::protocol::tcp&);
void update_context(libpacket::protocol::ipv4&,
                    const libpacket::protocol::udp&);

void update_context(libpacket::protocol::ipv6&,
                    const libpacket::protocol::tcp&);
void update_context(libpacket::protocol::ipv6&,
                    const libpacket::protocol::udp&);

void update_length(libpacket::protocol::ipv4&, uint16_t);
void update_length(libpacket::protocol::ipv6&, uint16_t);

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags, const libpacket::protocol::ipv4&);
flags update_packet_type(flags flags, const libpacket::protocol::ipv6&);

using header_lengths = packetio::packet::header_lengths;
void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::ipv4&);
void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::ipv6&);

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_IP_HPP_ */
