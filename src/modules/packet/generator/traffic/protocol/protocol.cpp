#include "packet/generator/traffic/protocol/protocol.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_length(libpacket::protocol::udp& udp, uint16_t length)
{
    set_udp_length(udp, length);
}

void update_length(libpacket::protocol::udp&, uint16_t);

flags update_packet_type(flags flags, const libpacket::protocol::tcp&)
{
    using namespace openperf::packetio::packet;
    return (flags | packet_type::protocol::tcp);
}

flags update_packet_type(flags flags, const libpacket::protocol::udp&)
{
    using namespace openperf::packetio::packet;
    return (flags | packet_type::protocol::udp);
}

void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::tcp&)
{
    lengths.layer4 += libpacket::protocol::tcp::protocol_length;
}

void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::udp&)
{
    lengths.layer4 += libpacket::protocol::udp::protocol_length;
}

} // namespace openperf::packet::generator::traffic::protocol
