#include "packet/analyzer/statistics/flow/header.hpp"
#include "packet/analyzer/statistics/flow/header_utils.hpp"
#include "packet/protocol/protocols.hpp"

namespace openperf::packet::analyzer::statistics::flow {

template <int Length, typename InputIt>
InputIt skip_header(InputIt cursor, InputIt end)
{
    if (cursor + Length <= end) { return (cursor + Length); }

    return (end);
}

template <typename Protocol, typename InputIt>
InputIt skip_header(InputIt cursor, InputIt end)
{
    if (cursor + Protocol::protocol_length <= end) {
        return (cursor + Protocol::protocol_length);
    }

    return (end);
}

const uint8_t* skip_layer2_headers(header::packet_type_flags flags,
                                   const uint8_t* cursor,
                                   const uint8_t* end)
{
    using ethernet = packetio::packet::packet_type::ethernet;

    switch ((flags & ethernet::mask).value) {
    case to_number(ethernet::ether):
        cursor = skip_header<libpacket::protocol::ethernet>(cursor, end);
        break;
    case to_number(ethernet::timesync):
        cursor = skip_header<34>(cursor, end);
        break;
    case to_number(ethernet::arp):
        cursor = skip_header<28>(cursor, end);
        break;
    case to_number(ethernet::vlan):
        cursor = skip_header<libpacket::protocol::ethernet>(cursor, end);
        cursor = skip_header<libpacket::protocol::vlan>(cursor, end);
        break;
    case to_number(ethernet::qinq):
        cursor = skip_header<libpacket::protocol::ethernet>(cursor, end);
        cursor = skip_header<libpacket::protocol::vlan>(cursor, end);
        cursor = skip_header<libpacket::protocol::vlan>(cursor, end);
        break;
    case to_number(ethernet::pppoe):
        cursor = skip_header<22>(cursor, end);
        break;
    case to_number(ethernet::fcoe):
        cursor = skip_header<36>(cursor, end);
        break;
    case to_number(ethernet::mpls): {
        cursor = skip_header<libpacket::protocol::ethernet>(cursor, end);
        while (cursor < end) {
            auto* mpls =
                reinterpret_cast<const libpacket::protocol::mpls*>(cursor);
            cursor = skip_header<libpacket::protocol::mpls>(cursor, end);
            if (get_mpls_bottom_of_stack(*mpls)) { break; }
        }
        break;
    }
    default:
        cursor = end;
    }

    return (cursor);
}

const uint8_t* skip_layer3_headers(header::packet_type_flags flags,
                                   const uint8_t* cursor,
                                   const uint8_t* end)
{
    using ip = packetio::packet::packet_type::ip;

    switch ((flags & ip::mask).value) {
    case to_number(ip::ipv4):
        cursor = skip_header<libpacket::protocol::ipv4>(cursor, end);
        break;
    case to_number(ip::ipv4_ext):
    case to_number(ip::ipv4_ext_unknown): {
        auto* ipv4 = reinterpret_cast<const libpacket::protocol::ipv4*>(cursor);
        cursor +=
            std::min(static_cast<ptrdiff_t>(get_ipv4_header_length(*ipv4)),
                     std::distance(cursor, end));
    } break;
    case to_number(ip::ipv6):
        cursor = skip_header<libpacket::protocol::ipv6>(cursor, end);
        break;
    case to_number(ip::ipv6_ext):
    case to_number(ip::ipv6_ext_unknown): {
        auto* ipv6 = reinterpret_cast<const libpacket::protocol::ipv6*>(cursor);
        cursor = skip_header<libpacket::protocol::ipv6>(cursor, end);
        if (cursor < end) {
            cursor = skip_ipv6_extension_headers(
                cursor, end, get_ipv6_next_header(*ipv6));
        }
        break;
    }
    default:
        cursor = end;
    }

    return (cursor);
}

const uint8_t* skip_layer4_headers(header::packet_type_flags flags,
                                   const uint8_t* cursor,
                                   const uint8_t* end)
{
    using protocol = packetio::packet::packet_type::protocol;

    switch ((flags & protocol::mask).value) {
    case to_number(protocol::tcp):
        cursor = skip_header<libpacket::protocol::tcp>(cursor, end);
        break;
    case to_number(protocol::udp):
        cursor = skip_header<libpacket::protocol::udp>(cursor, end);
        break;
    case to_number(protocol::sctp):
        cursor = skip_header<12>(cursor, end);
        break;
    case to_number(protocol::icmp):
        cursor = skip_header<8>(cursor, end);
        break;
    case to_number(protocol::igmp):
        cursor = skip_header<8>(cursor, end);
        break;
    default:
        cursor = end;
    }

    return (cursor);
}

uint16_t header_length(header::packet_type_flags flags,
                       const uint8_t pkt[],
                       uint16_t pkt_len)
{
    const auto end = pkt + pkt_len;
    auto cursor = skip_layer2_headers(flags, pkt, end);
    cursor = skip_layer3_headers(flags, cursor, end);
    cursor = skip_layer4_headers(flags, cursor, end);

    return (static_cast<uint16_t>(std::distance(pkt, cursor)));
}

} // namespace openperf::packet::analyzer::statistics::flow
