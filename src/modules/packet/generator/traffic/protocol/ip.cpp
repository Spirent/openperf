#include "packet/generator/traffic/protocol/ip.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::ipv4& ip,
                    const libpacket::protocol::tcp&)
{
    set_ipv4_protocol(ip, traffic::protocol::ip::tcp);
}

void update_context(libpacket::protocol::ipv4& ip,
                    const libpacket::protocol::udp&)
{
    set_ipv4_protocol(ip, traffic::protocol::ip::udp);
}

void update_context(libpacket::protocol::ipv6& ip,
                    const libpacket::protocol::tcp&)
{
    set_ipv6_next_header(ip, traffic::protocol::ip::tcp);
}

void update_context(libpacket::protocol::ipv6& ip,
                    const libpacket::protocol::udp&)
{
    set_ipv6_next_header(ip, traffic::protocol::ip::udp);
}

void update_length(libpacket::protocol::ipv4& ip, uint16_t length)
{
    set_ipv4_total_length(ip, length);
}

void update_length(libpacket::protocol::ipv6& ip, uint16_t length)
{
    const auto hdr_length = libpacket::protocol::ipv6::protocol_length;
    set_ipv6_payload_length(ip, length > hdr_length ? length - hdr_length : 0);
}

} // namespace openperf::packet::generator::traffic::protocol