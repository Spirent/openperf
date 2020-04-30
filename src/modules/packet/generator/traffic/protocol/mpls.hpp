#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::mpls&,
                    const libpacket::protocol::mpls&);

template <typename Next>
void update_context(libpacket::protocol::mpls& mpls, const Next&)
{
    static_assert(!std::is_same_v<Next, libpacket::protocol::mpls>);

    set_mpls_bottom_of_stack(mpls, true);
}

using flags = packetio::packet::packet_type::flags;
flags update_packet_type(flags flags, const libpacket::protocol::mpls&);

using header_lengths = packetio::packet::header_lengths;
void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::mpls&);

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_ */
