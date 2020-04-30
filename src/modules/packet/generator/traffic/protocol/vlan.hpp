#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv4&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv6&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::vlan&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::mpls&);

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags, const libpacket::protocol::vlan&);

using header_lengths = packetio::packet::header_lengths;
void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::vlan&);

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_ */
