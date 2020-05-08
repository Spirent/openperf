#include <cstring>

#include "catch.hpp"

#include "packet/generator/traffic/packet_template.hpp"

using namespace openperf::packet::generator::traffic;
using namespace openperf::packetio::packet;

template <typename Flag, typename Enum> bool flag_is_set(Flag f, Enum e)
{
    return ((f & e) == Flag(e));
}

void check_contents(const packet_template& pt,
                    unsigned exp_count,
                    unsigned hdr_len)
{
    /* zip means modifiers are expanded in parallel */
    REQUIRE(pt.size() == exp_count);

    /* packet templates are iterable */
    REQUIRE(std::count_if(std::begin(pt),
                          std::end(pt),
                          [](const auto&) { return (true); })
            == exp_count);

    /*
     * All headers should be unique. This is a little harder to verify
     * as the iterator returns a pointer.  So, iterate through the
     * container, copy the current header, and compare it to all the
     * others.
     */
    auto buffer = std::vector<uint8_t>{};

    std::for_each(std::begin(pt), std::end(pt), [&](const auto& hdr) {
        buffer.clear();
        std::copy_n(hdr, hdr_len, std::back_inserter(buffer));
        auto match = 0;
        auto mismatch = 0;
        std::for_each(std::begin(pt), std::end(pt), [&](const auto& chk) {
            if (std::memcmp(chk, buffer.data(), hdr_len)) {
                mismatch++;
            } else {
                match++;
            }
        });
        REQUIRE(match == 1);
        REQUIRE(mismatch == exp_count - 1);
    });
}

struct ethernet_ipv4_udp
{
    libpacket::protocol::ethernet ethernet;
    libpacket::protocol::ipv4 ipv4;
    libpacket::protocol::udp udp;
};

struct qinq_ipv6_tcp
{
    libpacket::protocol::ethernet ethernet;
    libpacket::protocol::vlan vlan1;
    libpacket::protocol::vlan vlan2;
    libpacket::protocol::ipv6 ipv6;
    libpacket::protocol::tcp tcp;
};

void set_mux(header::config_instance& config, header::modifier_mux mux)
{
    const auto set_mux = [&](auto& config) { config.mux = mux; };
    std::visit(set_mux, config);
}

TEST_CASE("traffic generator packet template,"
          "[packet_generator]")
{

    SECTION("simple expansion,")
    {
        /* Header Configuration #1 */
        auto header_configs1 = header::config_container{};
        header_configs1.push_back(header::ethernet_config{});

        auto ipv4_config = header::ipv4_config{};
        set_ipv4_defaults(ipv4_config.header);
        header_configs1.push_back(ipv4_config);

        auto udp_config = header::udp_config{};
        set_udp_defaults(udp_config.header);
        header_configs1.push_back(udp_config);

        /* Header Configuration #2 */
        auto header_configs2 = header::config_container{};
        header_configs2.push_back(header::ethernet_config{});
        header_configs2.push_back(header::vlan_config{});
        header_configs2.push_back(header::vlan_config{});

        auto ipv6_config = header::ipv6_config{};
        set_ipv6_defaults(ipv6_config.header);
        header_configs2.push_back(ipv6_config);

        auto tcp_config = header::tcp_config{};
        set_tcp_defaults(tcp_config.header);
        header_configs2.push_back(tcp_config);

        SECTION("expansion succeeds,")
        {
            auto pt1 =
                packet_template(header_configs1, header::modifier_mux::zip);
            REQUIRE(pt1.size() == 1);

            auto pt2 = packet_template(header_configs2,
                                       header::modifier_mux::cartesian);
            REQUIRE(pt2.size() == 1);

            SECTION("header lengths are valid,")
            {
                auto hdr1_lens = pt1.header_lengths();
                REQUIRE(hdr1_lens.layer2
                        == sizeof(libpacket::protocol::ethernet));
                REQUIRE(hdr1_lens.layer3 == sizeof(libpacket::protocol::ipv4));
                REQUIRE(hdr1_lens.layer4 == sizeof(libpacket::protocol::udp));

                auto hdr2_lens = pt2.header_lengths();
                REQUIRE(hdr2_lens.layer2
                        == sizeof(libpacket::protocol::ethernet)
                               + 2 * sizeof(libpacket::protocol::vlan));
                REQUIRE(hdr2_lens.layer3 == sizeof(libpacket::protocol::ipv6));
                REQUIRE(hdr2_lens.layer4 == sizeof(libpacket::protocol::tcp));
            }

            SECTION("header flags are valid,")
            {
                /* verify hdr flags */
                auto hdr1_flags = pt1.header_flags();
                REQUIRE(flag_is_set(hdr1_flags, packet_type::ethernet::ether));
                REQUIRE(flag_is_set(hdr1_flags, packet_type::ip::ipv4));
                REQUIRE(flag_is_set(hdr1_flags, packet_type::protocol::udp));

                auto hdr2_flags = pt2.header_flags();
                REQUIRE(flag_is_set(hdr2_flags, packet_type::ethernet::qinq));
                REQUIRE(flag_is_set(hdr2_flags, packet_type::ip::ipv6));
                REQUIRE(flag_is_set(hdr2_flags, packet_type::protocol::tcp));
            }

            SECTION("pseudoheader checksum is set,")
            {
                /*
                 * Packet template expansion writes the pseudoheader checksum
                 * into the payload checksum field.
                 */
                auto hdr1 = reinterpret_cast<const ethernet_ipv4_udp*>(pt1[0]);
                REQUIRE(get_udp_checksum(hdr1->udp) != 0);

                auto hdr2 = reinterpret_cast<const qinq_ipv6_tcp*>(pt2[0]);
                REQUIRE(get_tcp_checksum(hdr2->tcp) != 0);
            }
        }
    }

    SECTION("modifier expansion,")
    {
        constexpr auto eth_mod_count = 2;
        constexpr auto ip_mod_count = 4;
        auto header_configs = header::config_container{};

        using mac_address = libpacket::type::mac_address;
        using ipv4_address = libpacket::type::ipv4_address;

        auto eth_config = header::ethernet_config{};
        auto eth_dst_mod = modifier::sequence_config<mac_address>{
            .first = "10.94.0.1.0.1", .count = eth_mod_count};
        eth_config.modifiers.emplace_back(
            libpacket::protocol::ethernet::get_field_name("destination"),
            eth_dst_mod);
        header_configs.push_back(eth_config);

        auto ip_config = header::ipv4_config{};
        auto ip_src_mod = modifier::sequence_config<ipv4_address>{
            .first = "198.15.0.1", .count = ip_mod_count};
        auto ip_dst_mod = modifier::sequence_config<ipv4_address>{
            .first = "198.16.0.1", .count = ip_mod_count};
        ip_config.modifiers.emplace_back(
            libpacket::protocol::ipv4::get_field_name("source"), ip_src_mod);
        ip_config.modifiers.emplace_back(
            libpacket::protocol::ipv4::get_field_name("destination"),
            ip_dst_mod);
        header_configs.push_back(ip_config);

        /*
         * Given the header above we have 4 possible combinations for expansion:
         * pkt: zip, ip header: zip -> 4 headers
         * pkt: zip, ip header: cartesian -> 16 headers
         * pkt: cartesian, ip header: zip - > 8 headers
         * pkt: cartesian, ip header: cartesian -> 32 headers
         */
        constexpr auto exp_zip_zip = 4;
        constexpr auto exp_zip_cartesian = 16;
        constexpr auto exp_cartesian_zip = 8;
        constexpr auto exp_cartesian_cartesian = 32;

        SECTION("zip headers; zip ip,")
        {
            set_mux(header_configs[1], header::modifier_mux::zip);
            auto pt =
                packet_template(header_configs, header::modifier_mux::zip);

            auto hdr_lens = pt.header_lengths();
            check_contents(pt, exp_zip_zip, hdr_lens.layer2 + hdr_lens.layer3);
        }

        SECTION("zip headers; multiply ip,")
        {
            set_mux(header_configs[1], header::modifier_mux::cartesian);
            auto pt =
                packet_template(header_configs, header::modifier_mux::zip);

            auto hdr_lens = pt.header_lengths();
            check_contents(
                pt, exp_zip_cartesian, hdr_lens.layer2 + hdr_lens.layer3);
        }

        SECTION("multiply headers; zip ip")
        {
            set_mux(header_configs[1], header::modifier_mux::zip);
            auto pt = packet_template(header_configs,
                                      header::modifier_mux::cartesian);

            auto hdr_lens = pt.header_lengths();
            check_contents(
                pt, exp_cartesian_zip, hdr_lens.layer2 + hdr_lens.layer3);
        }

        SECTION("multiply headers; multiply ip")
        {
            set_mux(header_configs[1], header::modifier_mux::cartesian);
            auto pt = packet_template(header_configs,
                                      header::modifier_mux::cartesian);

            auto hdr_lens = pt.header_lengths();
            check_contents(
                pt, exp_cartesian_cartesian, hdr_lens.layer2 + hdr_lens.layer3);
        }
    }
}
