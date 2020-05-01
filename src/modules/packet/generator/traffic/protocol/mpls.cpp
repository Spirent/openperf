#include "packet/generator/traffic/protocol/mpls.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::mpls& mpls,
                    const libpacket::protocol::mpls&) noexcept
{
    set_mpls_bottom_of_stack(mpls, false);
}

flags update_packet_type(flags flags, const libpacket::protocol::mpls&) noexcept
{
    using namespace openperf::packetio::packet;
    return ((flags & ~packet_type::ethernet::mask)
            | packet_type::ethernet::mpls);
}

header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::mpls&) noexcept
{
    lengths.layer2 += libpacket::protocol::mpls::protocol_length;
    return (lengths);
}

} // namespace openperf::packet::generator::traffic::protocol
