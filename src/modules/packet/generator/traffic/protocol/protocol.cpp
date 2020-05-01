#include "packet/generator/traffic/protocol/protocol.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_length(libpacket::protocol::udp& udp, uint16_t length) noexcept
{
    set_udp_length(udp, length);
}

flags update_packet_type(flags flags, const libpacket::protocol::tcp&) noexcept
{
    using namespace openperf::packetio::packet;
    return (flags | packet_type::protocol::tcp);
}

flags update_packet_type(flags flags, const libpacket::protocol::udp&) noexcept
{
    using namespace openperf::packetio::packet;
    return (flags | packet_type::protocol::udp);
}

header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::tcp&) noexcept
{
    lengths.layer4 += libpacket::protocol::tcp::protocol_length;
    return (lengths);
}

header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::udp&) noexcept
{
    lengths.layer4 += libpacket::protocol::udp::protocol_length;
    return (lengths);
}

} // namespace openperf::packet::generator::traffic::protocol
