#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_PROTOCOL_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_PROTOCOL_HPP_

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_length(libpacket::protocol::udp&, uint16_t) noexcept;

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags, const libpacket::protocol::tcp&) noexcept;
flags update_packet_type(flags flags, const libpacket::protocol::udp&) noexcept;

using header_lengths = packetio::packet::header_lengths;
header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::tcp&) noexcept;
header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::udp&) noexcept;

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_PROTOCOL_HPP_ */
