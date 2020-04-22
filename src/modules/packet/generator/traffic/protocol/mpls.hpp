#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::mpls&,
                    const libpacket::protocol::mpls&);

template <typename Next>
void update_context(libpacket::protocol::mpls& mpls, const Next&)
{
    static_assert(!std::is_same_v<Next, libpacket::protocol::mpls>);

    set_mpls_bottom_of_stack(mpls, true);
}

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_MPLS_HPP_ */
