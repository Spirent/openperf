#include <map>
#include <utility>

#include "catch.hpp"

#include "packetio/bpf/bpf.hpp"
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
    SECTION("good")
    {
        REQUIRE_NOTHROW(openperf::packetio::bpf::bpf("length == 1000"));
        REQUIRE_NOTHROW(openperf::packetio::bpf::bpf(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2"));
        REQUIRE_NOTHROW(openperf::packetio::bpf::bpf(
            "ip src 10.0.0.1 and ip dst 10.0.0.2"));
        REQUIRE_NOTHROW(openperf::packetio::bpf::bpf(
            "ip6 src 2001::1 and ip6 dst 2001::2"));
    }
    SECTION("bad")
    {
        REQUIRE_THROWS_AS(
            openperf::packetio::bpf::bpf("length == BAD_LENGTH_VALUE"),
            std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packetio::bpf::bpf(
                              "ether src 10:0:0 and ether dst 10:0:0:0:0:2"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packetio::bpf::bpf("ip src 2001::1"),
                          std::invalid_argument);
        REQUIRE_THROWS_AS(openperf::packetio::bpf::bpf("ip6 src 10.0.0.1"),
                          std::invalid_argument);
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
        auto prog = openperf::packetio::bpf::bpf_compile(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packetio::bpf::bpf_jit(
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
        auto prog = openperf::packetio::bpf::bpf_compile(
            "ip src 1.0.0.1 and ip dst 1.0.0.2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packetio::bpf::bpf_jit(
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
        auto prog = openperf::packetio::bpf::bpf_compile(
            "ip6 src 2001::1 and ip6 dst 2001::2");
        REQUIRE(prog);

        // Validate filter program
        REQUIRE(op_bpf_validate(prog->bf_insns, prog->bf_len));

        // JIT compile the program
        auto jit = openperf::packetio::bpf::bpf_jit(
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

        openperf::packetio::bpf::bpf bpf(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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

        openperf::packetio::bpf::bpf bpf(
            "signature and ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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

        openperf::packetio::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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
        }

        SECTION("match all")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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

        openperf::packetio::bpf::bpf bpf(
            "signature streamid 1-5 and ether src 10:0:0:0:0:1 and ether dst "
            "10:0:0:0:0:2");

        SECTION("no match")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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
        }

        SECTION("match some")
        {
            std::vector<openperf::packetio::packet::packet_buffer*> packets;
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
        openperf::packetio::bpf::bpf bpf;

        BENCHMARK("all, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf(
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        BENCHMARK("ether, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf(
            "ether src 10:0:0:0:0:2 and ether dst 10:0:0:0:0:1");

        BENCHMARK("ether, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf("ip src 1.0.0.1 and ip dst 1.0.0.2");

        BENCHMARK("ip, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf("ip src 1.0.0.2 and ip dst 1.0.0.1");

        BENCHMARK("ip, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf("signature");

        uint32_t stream_id = 1;
        for (auto& mock : packets_mock) {
            mock->signature_stream_id = stream_id++;
        }

        BENCHMARK("signature, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }
        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result != 0);
        });

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("signature, no match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }
        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf("not signature");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }
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
        }
        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:1 and ether dst 10:0:0:0:0:2");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature ether, match")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

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
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }

    {
        openperf::packetio::bpf::bpf bpf(
            "not signature and "
            "ether src 10:0:0:0:0:2 and ether dst 10:0:0:0:0:1");

        for (auto& mock : packets_mock) { mock->signature_stream_id = {}; }

        BENCHMARK("not signature ether, no match (bad mac)")
        {
            bpf.exec_burst(packets.data(), results.data(), packets.size());
        }

        std::for_each(results.begin(), results.end(), [&](auto& result) {
            REQUIRE(result == 0);
        });
    }
}
