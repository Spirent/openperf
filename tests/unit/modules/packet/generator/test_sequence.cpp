#include "catch.hpp"

#include "packet/generator/traffic/sequence.hpp"
#include "packet/generator/traffic/protocol/all.hpp"

using namespace openperf::packet::generator::traffic;

using ipv4_address = libpacket::type::ipv4_address;
using ipv6_address = libpacket::type::ipv6_address;
using mac_address = libpacket::type::mac_address;

header::config_container get_ipv4_packet_config(unsigned mod_count)
{
    auto ipv4_dst_config = modifier::sequence_config<ipv4_address>{
        .first = "198.18.16.1", .count = mod_count};
    auto ipv4_src_config = modifier::sequence_config<ipv4_address>{
        .first = "198.18.15.1", .count = mod_count};
    auto mac_config = modifier::sequence_config<mac_address>{
        .first = "10.94.0.01.00.01", .count = mod_count};

    auto header_configs = header::config_container{};

    auto eth_config = header::ethernet_config{};
    set_ethernet_source(eth_config.header, "00.00.01.00.00.01");
    eth_config.modifiers.emplace_back(
        libpacket::protocol::ethernet::get_field_name("destination"),
        mac_config);
    header_configs.push_back(eth_config);

    auto ipv4_config = header::ipv4_config{.mux = header::modifier_mux::zip};
    set_ipv4_defaults(ipv4_config.header);
    ipv4_config.modifiers.emplace_back(
        libpacket::protocol::ipv4::get_field_name("destination"),
        ipv4_dst_config);
    ipv4_config.modifiers.emplace_back(
        libpacket::protocol::ipv4::get_field_name("source"), ipv4_src_config);
    header_configs.push_back(ipv4_config);

    return (header::update_context_fields(std::move(header_configs)));
}

header::config_container get_ipv6_packet_config(unsigned mod_count)
{
    auto ipv6_dst_config = modifier::sequence_config<ipv6_address>{
        .first = "2001:db9::1", .count = mod_count};
    auto ipv6_src_config = modifier::sequence_config<ipv6_address>{
        .first = "2001:db8::1", .count = mod_count};
    auto mac_dst_config = modifier::sequence_config<mac_address>{
        .first = "02:04:06:08:a0:b0", .count = mod_count};

    auto header_configs = header::config_container{};

    auto eth_config = header::ethernet_config{};
    set_ethernet_source(eth_config.header, "00.00.02.00.00.02");
    eth_config.modifiers.emplace_back(
        libpacket::protocol::ethernet::get_field_name("destination"),
        mac_dst_config);
    header_configs.push_back(eth_config);

    auto ipv6_config = header::ipv6_config{.mux = header::modifier_mux::zip};
    set_ipv6_defaults(ipv6_config.header);
    ipv6_config.modifiers.emplace_back(
        libpacket::protocol::ipv6::get_field_name("destination"),
        ipv6_dst_config);
    ipv6_config.modifiers.emplace_back(
        libpacket::protocol::ipv6::get_field_name("source"), ipv6_src_config);
    header_configs.push_back(ipv6_config);

    return (header::update_context_fields(std::move(header_configs)));
}

void check_length_sum(const sequence& seq, size_t n)
{
    REQUIRE(n < seq.size());

    auto len_sum = std::accumulate(std::begin(seq),
                                   std::next(std::begin(seq), n),
                                   0UL,
                                   [](size_t lhs, const auto& rhs) {
                                       return (lhs + std::get<uint16_t>(rhs));
                                   });

    REQUIRE(seq.sum_packet_lengths(n) == len_sum);
}

bool is_ipv4_header(const uint8_t* ptr)
{
    auto eth = reinterpret_cast<const libpacket::protocol::ethernet*>(ptr);
    return (libpacket::protocol::get_ethernet_ether_type(*eth)
            == protocol::ethernet::ether_type_ipv4);
}

void check_ipv4_count(const sequence& seq, size_t exp_ipv4_headers)
{
    auto nb_ipv4_headers =
        std::count_if(std::begin(seq), std::end(seq), [](const auto& tuple) {
            return (is_ipv4_header(std::get<const uint8_t*>(tuple)));
        });
    REQUIRE(nb_ipv4_headers == exp_ipv4_headers);
}

auto range(size_t n)
{
    auto r = std::vector<size_t>(n);
    std::iota(std::begin(r), std::end(r), 0);
    return (r);
}

TEST_CASE("packet generator sequences", "[packet_generator]")
{
    constexpr auto ipv4_mod_count = 4;
    constexpr auto ipv6_mod_count = 8;

    const auto ipv4_pkt_lengths = length_container{64, 512};
    const auto ipv6_pkt_lengths = length_container{1280};

    constexpr auto ipv4_weight = 2;
    constexpr auto ipv6_weight = 1;

    /*
     * Note: since packet templates and lengths can be large vectors, the
     * constructors take ownership of them instead of making copies.
     */
    auto ipv4_template = packet_template(get_ipv4_packet_config(ipv4_mod_count),
                                         header::modifier_mux::zip);
    auto ipv4_lengths = length_template(length_container(ipv4_pkt_lengths));

    auto ipv6_template = packet_template(get_ipv6_packet_config(ipv6_mod_count),
                                         header::modifier_mux::zip);
    auto ipv6_lengths = length_template(length_container(ipv6_pkt_lengths));

    auto definitions = definition_container{};
    definitions.emplace_back(std::move(ipv4_template),
                             std::move(ipv4_lengths),
                             ipv4_weight,
                             std::nullopt);
    definitions.emplace_back(std::move(ipv6_template),
                             std::move(ipv6_lengths),
                             ipv6_weight,
                             std::nullopt);

    SECTION("properties,")
    {
        auto defs = definitions;
        auto seq = sequence::round_robin_sequence(std::move(defs));

        REQUIRE(seq.flow_count() == ipv4_mod_count + ipv6_mod_count);
        REQUIRE(seq.max_packet_length() == 1280);
        REQUIRE(!seq.has_signature_config());
    }

    SECTION("round-robin,")
    {
        auto defs = definitions;
        auto seq = sequence::round_robin_sequence(std::move(defs));

        auto flow_lcm = std::lcm(ipv4_mod_count, ipv6_mod_count);
        REQUIRE(seq.size() == flow_lcm * (ipv4_weight + ipv6_weight));

        auto pkt_len_sum =
            std::accumulate(std::begin(seq),
                            std::end(seq),
                            0UL,
                            [](size_t lhs, const auto& rhs) {
                                return (lhs + std::get<uint16_t>(rhs));
                            });
        REQUIRE(seq.sum_packet_lengths() == pkt_len_sum);

        for (auto n : range(seq.size())) { check_length_sum(seq, n); }

        check_ipv4_count(seq, flow_lcm * ipv4_weight);
    }

    SECTION("sequential,")
    {
        auto defs = definitions;
        auto seq = sequence::sequential_sequence(std::move(defs));

        REQUIRE(seq.size()
                == (ipv4_mod_count * ipv4_weight)
                       + (ipv6_mod_count * ipv6_weight));

        auto pkt_len_sum =
            std::accumulate(std::begin(seq),
                            std::end(seq),
                            0UL,
                            [](size_t lhs, const auto& rhs) {
                                return (lhs + std::get<uint16_t>(rhs));
                            });
        REQUIRE(seq.sum_packet_lengths() == pkt_len_sum);

        for (auto n : range(seq.size())) { check_length_sum(seq, n); }

        check_ipv4_count(seq, ipv4_mod_count * ipv4_weight);
    }
}
