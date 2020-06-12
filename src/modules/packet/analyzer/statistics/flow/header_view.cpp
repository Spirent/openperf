#include <netinet/in.h>

#include "packet/analyzer/statistics/flow/header_view.hpp"
#include "packet/analyzer/statistics/flow/header_utils.hpp"

namespace openperf::packet::analyzer::statistics::flow {

using packet_type_flags = packetio::packet::packet_type::flags;

template <typename Protocol, typename InputIt>
InputIt store_header(InputIt cursor,
                     InputIt end,
                     std::vector<header_view::header_variant>& headers)
{
    if (cursor == end) { return (end); }

    if (cursor + Protocol::protocol_length <= end) {
        headers.push_back(reinterpret_cast<const Protocol*>(cursor));
        return (cursor + Protocol::protocol_length);
    }

    headers.emplace_back(
        std::basic_string_view<uint8_t>(cursor, std::distance(cursor, end)));
    return (end);
}

bool mpls_bottom_of_stack(const header_view::header_variant& header)
{
    auto* mpls = std::get<const libpacket::protocol::mpls*>(header);
    return (get_mpls_bottom_of_stack(*mpls));
}

const uint8_t*
parse_layer2_headers(packet_type_flags flags,
                     const uint8_t* cursor,
                     const uint8_t* end,
                     std::vector<header_view::header_variant>& headers)
{
    using ethernet = packetio::packet::packet_type::ethernet;

    switch ((flags & ethernet::mask).value) {
    case to_number(ethernet::ether):
        cursor =
            store_header<libpacket::protocol::ethernet>(cursor, end, headers);
        break;
    case to_number(ethernet::vlan):
        cursor =
            store_header<libpacket::protocol::ethernet>(cursor, end, headers);
        cursor = store_header<libpacket::protocol::vlan>(cursor, end, headers);
        break;
    case to_number(ethernet::qinq):
        cursor =
            store_header<libpacket::protocol::ethernet>(cursor, end, headers);
        cursor = store_header<libpacket::protocol::vlan>(cursor, end, headers);
        cursor = store_header<libpacket::protocol::vlan>(cursor, end, headers);
        break;
    case to_number(ethernet::mpls):
        cursor =
            store_header<libpacket::protocol::ethernet>(cursor, end, headers);
        cursor = store_header<libpacket::protocol::mpls>(cursor, end, headers);
        while (cursor != end && !mpls_bottom_of_stack(headers.back())) {
            cursor =
                store_header<libpacket::protocol::mpls>(cursor, end, headers);
        }
        break;
    default:
        headers.emplace_back(std::basic_string_view<uint8_t>(
            cursor, std::distance(cursor, end)));
        cursor = end;
    }

    return (cursor);
}

const uint8_t*
parse_layer3_headers(packet_type_flags flags,
                     const uint8_t* cursor,
                     const uint8_t* end,
                     std::vector<header_view::header_variant>& headers)
{
    using ip = packetio::packet::packet_type::ip;

    switch ((flags & ip::mask).value) {
    case to_number(ip::ipv4):
        cursor = store_header<libpacket::protocol::ipv4>(cursor, end, headers);
        break;
    case to_number(ip::ipv4_ext):
    case to_number(ip::ipv4_ext_unknown):
        cursor = store_header<libpacket::protocol::ipv4>(cursor, end, headers);
        if (cursor < end) {
            auto* ipv4 =
                std::get<const libpacket::protocol::ipv4*>(headers.back());
            cursor +=
                std::min(static_cast<ptrdiff_t>(get_ipv4_header_length(*ipv4)),
                         std::distance(cursor, end));
        }
        break;
    case to_number(ip::ipv6):
        cursor = store_header<libpacket::protocol::ipv6>(cursor, end, headers);
        break;
    case to_number(ip::ipv6_ext):
    case to_number(ip::ipv6_ext_unknown):
        cursor = store_header<libpacket::protocol::ipv6>(cursor, end, headers);
        if (cursor < end) {
            auto* ipv6 =
                std::get<const libpacket::protocol::ipv6*>(headers.back());
            cursor = skip_ipv6_extension_headers(
                cursor, end, get_ipv6_next_header(*ipv6));
        }
        break;
    default:
        headers.emplace_back(std::basic_string_view<uint8_t>(
            cursor, std::distance(cursor, end)));
        cursor = end;
    }

    return (cursor);
}

const uint8_t*
parse_layer4_headers(packet_type_flags flags,
                     const uint8_t* cursor,
                     const uint8_t* end,
                     std::vector<header_view::header_variant>& headers)
{
    using protocol = packetio::packet::packet_type::protocol;

    switch ((flags & protocol::mask).value) {
    case to_number(protocol::tcp):
        cursor = store_header<libpacket::protocol::tcp>(cursor, end, headers);
        break;
    case to_number(protocol::udp):
        cursor = store_header<libpacket::protocol::udp>(cursor, end, headers);
        break;
    default:
        headers.emplace_back(std::basic_string_view<uint8_t>(
            cursor, std::distance(cursor, end)));
        cursor = end;
    }

    return (cursor);
}

std::vector<header_view::header_variant>
parse_headers(packet_type_flags flags, const uint8_t* pkt, uint16_t pkt_len)
{
    auto headers = std::vector<header_view::header_variant>{};

    const auto end = pkt + pkt_len;
    auto cursor = parse_layer2_headers(flags, pkt, end, headers);
    cursor = parse_layer3_headers(flags, cursor, end, headers);
    parse_layer4_headers(flags, cursor, end, headers);

    return (headers);
}

header_view::header_view(const header& header)
    : m_headers(
        parse_headers(header.flags, header.data.data(), max_header_length))
{}

size_t header_view::size() const { return (m_headers.size()); }

header_view::header_variant header_view::operator[](size_t idx) const
{
    return (m_headers[idx]);
}

header_view::const_iterator header_view::begin() const
{
    return (iterator(*this));
}

header_view::const_iterator header_view::end() const
{
    return (iterator(*this, size()));
}

} // namespace openperf::packet::analyzer::statistics::flow
