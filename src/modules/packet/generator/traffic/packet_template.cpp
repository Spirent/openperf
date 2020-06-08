#include "packet/generator/traffic/packet_template.hpp"
#include "packet/generator/traffic/header/utils.hpp"

namespace openperf::packet::generator::traffic {

inline uint32_t fold64(uint64_t sum)
{
    sum = (sum >> 32) + (sum & 0xffffffff);
    sum += sum >> 32;
    return (static_cast<uint32_t>(sum));
}

inline uint16_t fold32(uint32_t sum)
{
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return (static_cast<uint16_t>(sum));
}

template <typename T>
static std::enable_if_t<sizeof(T) % sizeof(uint32_t) == 0, uint16_t>
do_checksum(const T& thing)
{
    const auto data = reinterpret_cast<const uint32_t*>(std::addressof(thing));
    const auto len = sizeof(T) / sizeof(uint32_t);
    const auto sum =
        std::accumulate(data, data + len, 0UL, [](uint64_t lhs, uint32_t rhs) {
            return (lhs + rhs);
        });

    return (fold64(fold32(sum)));
}

static uint16_t
get_pseudoheader_checksum(const libpacket::protocol::ipv4& ipv4) noexcept
{
    using namespace libpacket::type;
    struct ipv4_pseudoheader
    {
        ipv4_address source;
        ipv4_address destination;
        endian::field<1> zero;
        endian::field<1> protocol;
        endian::field<2> total_length;
    };

    auto phdr = ipv4_pseudoheader{.source = ipv4.source,
                                  .destination = ipv4.destination,
                                  .zero = 0,
                                  .protocol = ipv4.protocol,
                                  .total_length = ipv4.total_length != 0
                                                      ? ipv4.total_length.load()
                                                            - sizeof(ipv4)
                                                      : 0};

    return (do_checksum(phdr));
}

static uint16_t
get_pseudoheader_checksum(const libpacket::protocol::ipv6& ipv6) noexcept
{
    using namespace libpacket::type;
    struct ipv6_pseudoheader
    {
        ipv6_address source;
        ipv6_address destination;
        endian::field<4> length;
        endian::field<1> protocol;
        endian::field<3> zero;
    };

    auto phdr = ipv6_pseudoheader{
        .source = ipv6.source,
        .destination = ipv6.destination,
        .length = static_cast<uint32_t>(ipv6.payload_length.load()),
        .protocol = ipv6.next_header,
        .zero = 0};

    return (do_checksum(phdr));
}

static void
maybe_set_pseudoheader_checksum(uint8_t header[],
                                packetio::packet::header_lengths hdr_lens,
                                packetio::packet::packet_type::flags hdr_flags)
{
    using namespace packetio::packet;

    if (hdr_flags & packet_type::protocol::udp) {
        auto udp = reinterpret_cast<libpacket::protocol::udp*>(
            header + hdr_lens.layer2 + hdr_lens.layer3);
        if (hdr_flags & packet_type::ip::ipv4) {
            auto ipv4 = reinterpret_cast<libpacket::protocol::ipv4*>(
                header + hdr_lens.layer2);
            set_udp_checksum(*udp, get_pseudoheader_checksum(*ipv4));
        } else {
            auto ipv6 = reinterpret_cast<libpacket::protocol::ipv6*>(
                header + hdr_lens.layer3);
            set_udp_checksum(*udp, get_pseudoheader_checksum(*ipv6));
        }
    } else if (hdr_flags & packet_type::protocol::tcp) {
        auto tcp = reinterpret_cast<libpacket::protocol::tcp*>(
            header + hdr_lens.layer2 + hdr_lens.layer3);
        if (hdr_flags & packet_type::ip::ipv4) {
            auto ipv4 = reinterpret_cast<libpacket::protocol::ipv4*>(
                header + hdr_lens.layer2);
            set_tcp_checksum(*tcp, get_pseudoheader_checksum(*ipv4));
        } else {
            auto ipv6 = reinterpret_cast<libpacket::protocol::ipv6*>(
                header + hdr_lens.layer3);
            set_tcp_checksum(*tcp, get_pseudoheader_checksum(*ipv6));
        }
    }
}

packet_template::packet_template(const header::config_container& configs,
                                 header::modifier_mux mux)
    : m_headers(header::make_headers(configs, mux))
    , m_hdr_lens(header::to_packet_header_lengths(configs))
    , m_flags(header::to_packet_type_flags(configs))
{
    std::for_each(
        std::begin(m_headers), std::end(m_headers), [&](const auto& pair) {
            maybe_set_pseudoheader_checksum(
                const_cast<uint8_t*>(pair.first), m_hdr_lens, m_flags);
        });
}

packetio::packet::header_lengths packet_template::header_lengths() const
{
    return (m_hdr_lens);
}

packetio::packet::packet_type::flags packet_template::header_flags() const
{
    return (m_flags);
}

size_t packet_template::size() const { return (m_headers.size()); }

packet_template::view_type packet_template::operator[](size_t idx) const
{
    assert(idx < size());

    return (m_headers[idx].first);
}

packet_template::iterator packet_template::begin() { return (iterator(*this)); }

packet_template::const_iterator packet_template::begin() const
{
    return (iterator(*this));
}

packet_template::iterator packet_template::end()
{
    return (iterator(*this, size()));
}

packet_template::const_iterator packet_template::end() const
{
    return (iterator(*this, size()));
}

void update_packet_header_lengths(
    const uint8_t header[],
    packetio::packet::header_lengths hdr_lens,
    packetio::packet::packet_type::flags hdr_flags,
    uint16_t pkt_len,
    uint8_t packet[])
{
    using namespace packetio::packet;
    using namespace libpacket::type::endian;

    const auto payload_len = pkt_len - (hdr_lens.layer2 + hdr_lens.layer3);

    if (hdr_flags & packet_type::ip::ipv4) {
        auto ipv4 = reinterpret_cast<libpacket::protocol::ipv4*>(
            packet + hdr_lens.layer2);
        set_ipv4_total_length(*ipv4, pkt_len - hdr_lens.layer2);
    }
    if (hdr_flags & packet_type::ip::ipv6) {
        auto ipv6 = reinterpret_cast<libpacket::protocol::ipv6*>(
            packet + hdr_lens.layer2);
        set_ipv6_payload_length(*ipv6, payload_len);
    }
    if (hdr_flags & packet_type::protocol::udp) {
        const auto udp_offset = hdr_lens.layer2 + hdr_lens.layer3;
        auto src_udp = reinterpret_cast<const libpacket::protocol::udp*>(
            header + udp_offset);
        auto dst_udp =
            reinterpret_cast<libpacket::protocol::udp*>(packet + udp_offset);
        set_udp_length(*dst_udp, payload_len);
        set_udp_checksum(
            *dst_udp,
            bswap(fold32(get_udp_checksum(*src_udp) + bswap(payload_len))));
    }
    if (hdr_flags & packet_type::protocol::tcp) {
        const auto tcp_offset = hdr_lens.layer2 + hdr_lens.layer3;
        auto src_tcp = reinterpret_cast<const libpacket::protocol::tcp*>(
            header + tcp_offset);
        auto dst_tcp =
            reinterpret_cast<libpacket::protocol::tcp*>(packet + tcp_offset);
        set_tcp_checksum(
            *dst_tcp,
            bswap(fold32(get_tcp_checksum(*src_tcp) + bswap(payload_len))));
    }
}

} // namespace openperf::packet::generator::traffic
