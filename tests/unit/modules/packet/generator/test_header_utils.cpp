#include "catch.hpp"

#include "packet/generator/traffic/header/utils.hpp"
#include "packet/generator/traffic/protocol/all.hpp"

using namespace openperf::packet::generator::traffic;

template <typename Header>
const Header* find_header(const header::config_container& container)
{
    auto cursor = std::begin(container), end = std::end(container);
    do {
        if (auto c = std::get_if<header::config<Header>>(&(*cursor))) {
            return (std::addressof(c->header));
        }
    } while (++cursor != end);

    return (nullptr);
}

TEST_CASE("packet generator header utils", "[packet_generator]")
{
    SECTION("context update,")
    {
        using namespace libpacket::protocol;

        SECTION("ethernet/ip/udp,")
        {
            auto header_configs = header::config_container{};
            header_configs.push_back(header::ethernet_config{});
            header_configs.push_back(header::ipv4_config{});
            header_configs.push_back(header::udp_config{});

            auto update = update_context_fields(std::move(header_configs));

            auto eth = find_header<libpacket::protocol::ethernet>(update);
            REQUIRE(get_ethernet_ether_type(*eth)
                    == protocol::ethernet::ether_type_ipv4);

            auto ip = find_header<libpacket::protocol::ipv4>(update);
            REQUIRE(get_ipv4_protocol(*ip) == protocol::ip::udp);
        }

        SECTION("ethernet/vlan/vlan/ipv6/tcp,")
        {
            auto header_configs = header::config_container{};
            header_configs.push_back(header::ethernet_config{});
            header_configs.push_back(header::vlan_config{});
            header_configs.push_back(header::vlan_config{});
            header_configs.push_back(header::ipv6_config{});
            header_configs.push_back(header::tcp_config{});

            auto update = update_context_fields(std::move(header_configs));

            auto eth = find_header<libpacket::protocol::ethernet>(update);
            REQUIRE(get_ethernet_ether_type(*eth)
                    == protocol::ethernet::ether_type_qinq);

            auto vlan1 = find_header<libpacket::protocol::vlan>(update);
            REQUIRE(get_vlan_ether_type(*vlan1)
                    == protocol::ethernet::ether_type_vlan);

            auto hack_config = header::config_container{};
            hack_config.push_back(update[2]);
            auto vlan2 = find_header<libpacket::protocol::vlan>(hack_config);
            REQUIRE(get_vlan_ether_type(*vlan2)
                    == protocol::ethernet::ether_type_ipv6);

            auto ipv6 = find_header<libpacket::protocol::ipv6>(update);
            REQUIRE(get_ipv6_next_header(*ipv6) == protocol::ip::tcp);
        }
    }
}
