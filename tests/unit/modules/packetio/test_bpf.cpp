#include <map>
#include <utility>

#include "catch.hpp"

#include "packetio/bpf/bpf.hpp"
#include <pcap.h>

#include "lib/packet/protocol/ethernet.hpp"
#include "lib/packet/protocol/ipv4.hpp"
#include "lib/packet/protocol/ipv6.hpp"

struct ethernet_ipv4
{
    libpacket::protocol::ethernet ethernet;
    libpacket::protocol::ipv4 ipv4;
};

struct ethernet_ipv6
{
    libpacket::protocol::ethernet ethernet;
    libpacket::protocol::ipv6 ipv6;
};

const uint16_t ETHERTYPE_IP = 0x0800;
const uint16_t ETHERTYPE_IPV6 = 0x86DD;

void create_ipv4_packet(uint8_t* data, size_t len,
                        const libpacket::type::mac_address &src_mac,
                        const libpacket::type::mac_address &dst_mac,
                        const libpacket::type::ipv4_address &src_ip,
                        const libpacket::type::ipv4_address &dst_ip)
{
    auto* packet = reinterpret_cast<ethernet_ipv4*>(data);

    assert(len >= (sizeof(*packet) + sizeof(uint32_t)));
    packet->ethernet.source = src_mac;
    packet->ethernet.destination = dst_mac;
    packet->ethernet.ether_type = ETHERTYPE_IP;
    libpacket::protocol::set_ipv4_version(packet->ipv4, 4);
    packet->ipv4.source = src_ip;
    packet->ipv4.destination = dst_ip;
    packet->ipv4.total_length =
        len - sizeof(packet->ethernet) - sizeof(uint32_t);
}

void create_ipv6_packet(uint8_t* data, size_t len,
                        const libpacket::type::mac_address &src_mac,
                        const libpacket::type::mac_address &dst_mac,
                        const libpacket::type::ipv6_address &src_ip,
                        const libpacket::type::ipv6_address &dst_ip)
{
    auto* packet = reinterpret_cast<ethernet_ipv6*>(data);

    assert(len >= (sizeof(packet) + sizeof(uint32_t)));
    packet->ethernet.source = src_mac;
    packet->ethernet.destination = dst_mac;
    packet->ethernet.ether_type = ETHERTYPE_IPV6;
    libpacket::protocol::set_ipv6_version(packet->ipv6, 6);
    packet->ipv6.source = src_ip;
    packet->ipv6.destination = dst_ip;
    packet->ipv6.payload_length =
        len - sizeof(packet) - sizeof(uint32_t);
}

TEST_CASE("bpf", "[bpf]")
{
    SECTION("jit, mac")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packetio::bpf::bpf_compile("ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");
        REQUIRE(prog);

        auto jit = openperf::packetio::bpf::bpf_jit(prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
                == 0);
        // Create packet matching filter
        create_ipv4_packet(buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv4_packet(buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()) == false);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()) == false);
    }

    SECTION("jit, ipv4")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packetio::bpf::bpf_compile("ip src 1.0.0.1 and ip dst 1.0.0.2");
        REQUIRE(prog);

        auto jit = openperf::packetio::bpf::bpf_jit(prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
                == 0);
        // Create packet matching filter
        create_ipv4_packet(buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv4_packet(buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()) == false);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()) == false);
    }

    SECTION("jit, ipv6")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv6_address("2001::1");
        const auto dst_ip = libpacket::type::ipv6_address("2001::2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packetio::bpf::bpf_compile("ip6 src 2001::1 and ip6 dst 2001::2");
        REQUIRE(prog);

        auto jit = openperf::packetio::bpf::bpf_jit(prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
                == 0);
        // Create packet matching filter
        create_ipv6_packet(buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv6_packet(buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()) == false);
        REQUIRE(op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()) == false);
    }
}
