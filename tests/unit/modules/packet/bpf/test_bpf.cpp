#include <map>
#include <utility>

#include "catch.hpp"

#include "packet/bpf/bpf.hpp"
#include "packet/bpf/bpf_build.hpp"
#include "packet/bpf/bpf_sink.hpp"
#include "packetio/mock_packet_buffer.hpp"
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

void create_ipv4_packet(uint8_t* data,
                        size_t len,
                        const libpacket::type::mac_address& src_mac,
                        const libpacket::type::mac_address& dst_mac,
                        const libpacket::type::ipv4_address& src_ip,
                        const libpacket::type::ipv4_address& dst_ip)
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

void create_ipv6_packet(uint8_t* data,
                        size_t len,
                        const libpacket::type::mac_address& src_mac,
                        const libpacket::type::mac_address& dst_mac,
                        const libpacket::type::ipv6_address& src_ip,
                        const libpacket::type::ipv6_address& dst_ip)
{
    auto* packet = reinterpret_cast<ethernet_ipv6*>(data);

    assert(len >= (sizeof(packet) + sizeof(uint32_t)));
    packet->ethernet.source = src_mac;
    packet->ethernet.destination = dst_mac;
    packet->ethernet.ether_type = ETHERTYPE_IPV6;
    libpacket::protocol::set_ipv6_version(packet->ipv6, 6);
    packet->ipv6.source = src_ip;
    packet->ipv6.destination = dst_ip;
    packet->ipv6.payload_length = len - sizeof(packet) - sizeof(uint32_t);
}

TEST_CASE("bpf", "[bpf]")
{
    SECTION("empty")
    {
        {
            openperf::packet::bpf::bpf bpf;
            REQUIRE(bpf.get_filter_flags() == 0);
        }
    }

    SECTION("libpcap")
    {
        {
            openperf::packet::bpf::bpf bpf("length == 1000");
            REQUIRE(bpf.get_filter_flags() == 0);
        }
        {
            openperf::packet::bpf::bpf bpf(
                "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");
            REQUIRE(bpf.get_filter_flags() == 0);
        }
        {
            openperf::packet::bpf::bpf bpf(
                "ip src 10.0.0.1 and ip dst 10.0.0.2");
            REQUIRE(bpf.get_filter_flags() == 0);
        }
        {
            openperf::packet::bpf::bpf bpf(
                "ip src 10.0.0.1 and ip dst 10.0.0.2");
            REQUIRE(bpf.get_filter_flags() == 0);
        }
        {
            openperf::packet::bpf::bpf bpf(
                "ip6 src 2001::1 and ip6 dst 2001::2");
            REQUIRE(bpf.get_filter_flags() == 0);
        }
    }

    SECTION("signature")
    {
        {
            openperf::packet::bpf::bpf bpf("signature");
            REQUIRE(bpf.get_filter_flags()
                    == openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE);
        }
        {
            openperf::packet::bpf::bpf bpf("signature streamid 1");
            REQUIRE(bpf.get_filter_flags()
                    == (openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                        | openperf::packet::bpf::
                            BPF_FILTER_FLAGS_SIGNATURE_STREAM_ID));
        }
    }

    SECTION("valid")
    {
        {
            openperf::packet::bpf::bpf bpf("valid ip chksum");
            REQUIRE(bpf.get_filter_flags()
                    == openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR);
        }
        {
            openperf::packet::bpf::bpf bpf("valid tcp chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR);
        }
        {
            openperf::packet::bpf::bpf bpf("valid udp chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR);
        }
        {
            openperf::packet::bpf::bpf bpf("valid icmp chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR);
        }
        {
            openperf::packet::bpf::bpf bpf("valid tcp chksum udp chksum");
            REQUIRE(bpf.get_filter_flags()
                    == (openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                        | openperf::packet::bpf::
                            BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR));
        }
        {
            openperf::packet::bpf::bpf bpf(
                "valid tcp chksum and valid udp chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == (openperf::packet::bpf::BPF_FILTER_FLAGS_AND
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                    | openperf::packet::bpf::
                        BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR));
        }
        {
            openperf::packet::bpf::bpf bpf("valid chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == (openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR
                    | openperf::packet::bpf::
                        BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR));
        }
        {
            openperf::packet::bpf::bpf bpf("valid prbs");
            REQUIRE(bpf.get_filter_flags()
                    == (openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                        | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR));
        }
        {
            openperf::packet::bpf::bpf bpf("not valid chksum");
            REQUIRE(
                bpf.get_filter_flags()
                == (openperf::packet::bpf::BPF_FILTER_FLAGS_NOT
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR
                    | openperf::packet::bpf::
                        BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR));
        }
        {
            openperf::packet::bpf::bpf bpf("not valid prbs");
            REQUIRE(bpf.get_filter_flags()
                    == (openperf::packet::bpf::BPF_FILTER_FLAGS_NOT
                        | openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                        | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR));
        }
    }

    SECTION("bad")
    {
        REQUIRE_THROWS_AS(
            openperf::packet::bpf::bpf("length == BAD_LENGTH_VALUE"),
            std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packet::bpf::bpf(
                              "ether src 10:0:0 and ether dst 10:0:0:0:0:2"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packet::bpf::bpf("ip src 2001::1"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packet::bpf::bpf("ip6 src 10.0.0.1"),
                          std::invalid_argument);
    }
}

TEST_CASE("bpf sink_feature_flags", "[bpf]")
{
    SECTION("none")
    {
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(0).value == 0);
    }

    SECTION("packet_type_decode")
    {
        auto expected = static_cast<int>(
            openperf::packetio::packet::sink_feature_flags::packet_type_decode);

        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR)
                    .value
                == expected);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR)
                    .value
                == expected);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR)
                    .value
                == expected);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR)
                    .value
                == expected);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR)
                    .value
                == expected);
    }

    SECTION("spirent_signature_decode")
    {
        auto expected =
            static_cast<int>(openperf::packetio::packet::sink_feature_flags::
                                 spirent_signature_decode);

        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE)
                    .value
                == expected);
    }

    SECTION("spirent_prbs_error_decode")
    {
        auto expected = (openperf::packetio::packet::sink_feature_flags::
                             spirent_signature_decode
                         | openperf::packetio::packet::sink_feature_flags::
                             spirent_prbs_error_detect);

        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR)
                    .value
                == expected.value);
    }

    SECTION("packet_type_decode and spirent_prbs_error_decode")
    {
        auto expected =
            (openperf::packetio::packet::sink_feature_flags::packet_type_decode
             | openperf::packetio::packet::sink_feature_flags::
                 spirent_signature_decode
             | openperf::packetio::packet::sink_feature_flags::
                 spirent_prbs_error_detect);

        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR)
                    .value
                == expected.value);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR)
                    .value
                == expected.value);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR)
                    .value
                == expected.value);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR)
                    .value
                == expected.value);
        REQUIRE(openperf::packet::bpf::bpf_sink_feature_flags(
                    openperf::packet::bpf::BPF_FILTER_FLAGS_SIGNATURE
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_PRBS_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_IP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_TCP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_UDP_CHKSUM_ERROR
                    | openperf::packet::bpf::BPF_FILTER_FLAGS_ICMP_CHKSUM_ERROR)
                    .value
                == expected.value);
    }
}

TEST_CASE("bpf w/ raw data", "[bpf]")
{
    SECTION("mac")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packet::bpf::bpf_compile(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packet::bpf::bpf_jit(
            nullptr, prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == 0);
        // Create packet matching filter
        create_ipv4_packet(
            buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv4_packet(
            buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == false);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == false);
    }

    SECTION("ipv4")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packet::bpf::bpf_compile(
            "ip src 1.0.0.1 and ip dst 1.0.0.2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packet::bpf::bpf_jit(
            nullptr, prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == 0);
        // Create packet matching filter
        create_ipv4_packet(
            buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv4_packet(
            buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == false);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == false);
    }

    SECTION("ipv6")
    {
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv6_address("2001::1");
        const auto dst_ip = libpacket::type::ipv6_address("2001::2");

        // Use libpcap to parse the filter string
        auto prog = openperf::packet::bpf::bpf_compile(
            "ip6 src 2001::1 and ip6 dst 2001::2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packet::bpf::bpf_jit(
            nullptr, prog->bf_insns, prog->bf_len);
        REQUIRE(jit);
        auto jit_func = *jit;

        std::array<uint8_t, 64> buf;
        std::fill(buf.begin(), buf.end(), 0);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == 0);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == 0);
        // Create packet matching filter
        create_ipv6_packet(
            buf.data(), buf.size(), src_mac, dst_mac, src_ip, dst_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size()));
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size()));
        // Create packet with src/dst reversed
        create_ipv6_packet(
            buf.data(), buf.size(), dst_mac, src_mac, dst_ip, src_ip);
        REQUIRE(op_bpfjit_filter(jit_func, buf.data(), buf.size(), buf.size())
                == false);
        REQUIRE(
            op_bpf_filter(prog->bf_insns, buf.data(), buf.size(), buf.size())
            == false);
    }
}

TEST_CASE("bpf w/ packet_buffer", "[bpf]")
{
    SECTION("mac")
    {
        using packet_data_t = std::vector<uint8_t>;
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");
        const int num_packets = 10;

        openperf::packet::bpf::bpf bpf(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                // Use reversed src/dst
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   dst_mac,
                                   src_mac,
                                   dst_ip,
                                   src_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == packets.size());
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result == 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == 0);
            std::for_each(filtered_packets.begin(),
                          filtered_packets.end(),
                          [](auto& packet) { REQUIRE(packet == nullptr); });
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == offset);
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result != 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == packets.size());
            for (size_t i = 0; i < packets.size(); ++i) {
                REQUIRE(packets[i] == filtered_packets[i]);
            }
        }
    }

    SECTION("signature")
    {
        using packet_data_t = std::vector<uint8_t>;
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");
        const int num_packets = 10;

        openperf::packet::bpf::bpf bpf(
            "signature and ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == packets.size());
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result == 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == 0);
            std::for_each(filtered_packets.begin(),
                          filtered_packets.end(),
                          [](auto& packet) { REQUIRE(packet == nullptr); });
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                mock->signature_stream_id = 1;
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == offset);
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result != 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == packets.size());
            for (size_t i = 0; i < packets.size(); ++i) {
                REQUIRE(packets[i] == filtered_packets[i]);
            }
        }
    }

    SECTION("not signature")
    {
        using packet_data_t = std::vector<uint8_t>;
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");
        const int num_packets = 10;

        openperf::packet::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                mock->signature_stream_id = 1;
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == packets.size());
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result == 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == 0);
            std::for_each(filtered_packets.begin(),
                          filtered_packets.end(),
                          [](auto& packet) { REQUIRE(packet == nullptr); });
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == offset);
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result != 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == packets.size());
            for (size_t i = 0; i < packets.size(); ++i) {
                REQUIRE(packets[i] == filtered_packets[i]);
            }
        }
    }

    SECTION("signature streamid 1-5")
    {
        using packet_data_t = std::vector<uint8_t>;
        const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
        const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
        const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
        const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");
        const int num_packets = 10;

        openperf::packet::bpf::bpf bpf(
            "signature streamid 1-5 and ether src 10:0:0:0:0:1 and ether dst "
            "10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            uint32_t ndx = 6;
            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                mock->signature_stream_id = ndx++;
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            for (size_t offset = 0; offset < packets.size(); ++offset) {
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == packets.size());
            }
            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            std::for_each(results.begin(), results.end(), [](auto& result) {
                REQUIRE(result == 0);
            });
            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == 0);
            std::for_each(filtered_packets.begin(),
                          filtered_packets.end(),
                          [](auto& result) { REQUIRE(result == nullptr); });
        }

        SECTION("match some")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
            std::vector<const openperf::packetio::packet::packet_buffer*>
                filtered_packets;
            std::vector<
                std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
                packets_mock;
            std::vector<std::unique_ptr<packet_data_t>> packets_data;
            std::vector<uint64_t> results;

            uint32_t ndx = 1;
            std::generate_n(std::back_inserter(packets), num_packets, [&]() {
                packets_data.emplace_back(std::make_unique<packet_data_t>());
                auto& data = packets_data.back();
                packets_mock.emplace_back(
                    std::make_unique<
                        openperf::packetio::packet::mock_packet_buffer>());
                auto& mock = packets_mock.back();
                data->resize(64);
                create_ipv4_packet(data->data(),
                                   data->size(),
                                   src_mac,
                                   dst_mac,
                                   src_ip,
                                   dst_ip);
                mock->data = data->data();
                mock->length = mock->data_length = data->size();
                mock->signature_stream_id = ndx++;
                return reinterpret_cast<
                    openperf::packetio::packet::packet_buffer*>(mock.get());
            });
            filtered_packets.resize(packets.size());
            results.resize(packets.size());

            ndx = 1;
            for (size_t offset = 0; offset < packets.size(); ++offset) {
                auto expected = ((ndx++) <= 5) ? offset : packets.size();
                REQUIRE(bpf.find_next(packets.data(), packets.size(), offset)
                        == expected);
            }

            REQUIRE(
                bpf.exec_burst(packets.data(), results.data(), packets.size())
                == packets.size());
            ndx = 1;
            std::for_each(results.begin(), results.end(), [&](auto& result) {
                REQUIRE((result != 0) == ((ndx++) <= 5));
            });

            REQUIRE(bpf.filter_burst(
                        packets.data(), filtered_packets.data(), packets.size())
                    == 5);
            std::for_each(
                filtered_packets.begin(),
                filtered_packets.begin() + 5,
                [&](auto& packet) {
                    auto stream_id =
                        openperf::packetio::packet::signature_stream_id(packet);
                    REQUIRE(stream_id.has_value());
                    REQUIRE((*stream_id <= 5));
                });
        }
    }
}

TEST_CASE("bpf benchmarks", "[bpf]")
{
    using packet_data_t = std::vector<uint8_t>;
    const auto src_mac = libpacket::type::mac_address("10:0:0:0:0:1");
    const auto dst_mac = libpacket::type::mac_address("10:0:0:0:0:2");
    const auto src_ip = libpacket::type::ipv4_address("1.0.0.1");
    const auto dst_ip = libpacket::type::ipv4_address("1.0.0.2");
    const int num_packets = 1000;

    std::vector<openperf::packetio::packet::packet_buffer*> packets;
    std::vector<std::unique_ptr<openperf::packetio::packet::mock_packet_buffer>>
        packets_mock;
    std::vector<std::unique_ptr<packet_data_t>> packets_data;
    std::vector<uint64_t> results;

    std::generate_n(std::back_inserter(packets), num_packets, [&]() {
        packets_data.emplace_back(std::make_unique<packet_data_t>());
        auto& data = packets_data.back();
        packets_mock.emplace_back(
            std::make_unique<openperf::packetio::packet::mock_packet_buffer>());
        auto& mock = packets_mock.back();
        data->resize(64);
        create_ipv4_packet(
            data->data(), data->size(), src_mac, dst_mac, src_ip, dst_ip);
        mock->data = data->data();
        mock->length = mock->data_length = data->size();
        return reinterpret_cast<openperf::packetio::packet::packet_buffer*>(
            mock.get());
    });
    results.resize(packets.size());

    {
        openperf::packet::bpf::bpf bpf;

        BENCHMARK("all, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        BENCHMARK("ether, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf(
            "ether src 10:0:0:0:0:2 and ether dst 10:0:0:0:0:1");

        BENCHMARK("ether, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf("ip src 1.0.0.1 and ip dst 1.0.0.2");

        BENCHMARK("ip, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf("ip src 1.0.0.2 and ip dst 1.0.0.1");

        BENCHMARK("ip, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf("signature");

        uint32_t stream_id = 1;
        for (auto& mock : packets_mock) {
            mock->signature_stream_id = stream_id++;
        }

        BENCHMARK("signature, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("signature, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf("not signature");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });

        uint32_t stream_id = 1;
        for (auto& mock : packets_mock) {
            mock->signature_stream_id = stream_id++;
        }

        BENCHMARK("not signature, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature ether, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });

        uint32_t stream_id = 1;
        for (auto& mock : packets_mock) {
            mock->signature_stream_id = ++stream_id;
        }

        BENCHMARK("not signature ether, no match (has sig)")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packet::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:2 and ether dst 10:0:0:0:0:1");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature ether, no match (bad mac)")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        };

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }
}
