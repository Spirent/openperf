#include <chrono>
#include <filesystem>
#include <arpa/inet.h>

#include "catch.hpp"

#include "packet/capture/capture_buffer.hpp"
#include "packetio/mock_packet_buffer.hpp"

using namespace openperf::packet::capture;
using namespace openperf::packetio::packets;

struct eth_hdr
{
    std::array<uint8_t, 6> ether_dhost;
    std::array<uint8_t, 6> ether_shost;
    uint16_t ether_type;
} __attribute__((packed));

const uint16_t ETHERTYPE_IP = 0x0800;

const uint32_t FCS_SIZE = 4;

struct ipv4_hdr
{
    uint8_t ver_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t packet_id;
    uint16_t fragment_offset;
    uint8_t ttl;
    uint8_t proto_id;
    uint16_t checksum;
    std::array<uint8_t, 4> src_addr;
    std::array<uint8_t, 4> dst_addr;
} __attribute__((__packed__));

const uint8_t IPPROT_EXP = 0xfd;

static uint8_t ipv4_ver_ihl(uint8_t ver, uint8_t len)
{
    len = (len / 4); // convert to 32-bit words
    return ((ver & 0xf) << 4) | (len & 0xf);
}

static void create_ipv4_packet(mock_packet_buffer& packet_buffer,
                               uint8_t* data,
                               size_t data_size,
                               size_t payload_size,
                               uint64_t timestamp)
{
    std::array<uint8_t, 4> src_ip{1, 0, 0, 1};
    std::array<uint8_t, 4> dst_ip{1, 0, 0, 2};
    std::array<uint8_t, 6> src_mac{0x10, 0, 0, 0, 0, 1};
    std::array<uint8_t, 6> dst_mac{0x10, 0, 0, 0, 0, 2};

    packet_buffer.data = data;
    packet_buffer.data_length = data_size;
    packet_buffer.rx_timestamp = timestamp;

    auto ptr = reinterpret_cast<uint8_t*>(packet_buffer.data);
    auto eth = reinterpret_cast<eth_hdr*>(ptr);
    eth->ether_type = htons(ETHERTYPE_IP);
    eth->ether_dhost = dst_mac;
    eth->ether_shost = src_mac;
    ptr += sizeof(*eth);
    packet_buffer.length += sizeof(*eth);
    auto ipv4 = reinterpret_cast<ipv4_hdr*>(ptr);
    memset(ipv4, 0, sizeof(*ipv4));
    ipv4->ver_ihl = ipv4_ver_ihl(4, 20);
    ipv4->total_length = ntohs(sizeof(*ipv4) + payload_size);
    ipv4->src_addr = src_ip;
    ipv4->dst_addr = dst_ip;
    ipv4->proto_id = IPPROT_EXP;
    ptr += sizeof(*ipv4);
    packet_buffer.length += sizeof(*ipv4);
    memset(ptr, 0, payload_size + FCS_SIZE);
    packet_buffer.length += (payload_size + FCS_SIZE);
}

void create_capture_buffers(
    std::vector<std::unique_ptr<capture_buffer_file>>& buffers, size_t count)
{
    buffers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        auto capture_filename =
            std::string("test-") + std::to_string(i) + std::string(".pcapng");
        buffers.emplace_back(
            std::make_unique<capture_buffer_file>(capture_filename));
        REQUIRE(std::filesystem::exists(capture_filename));
    }
}

std::unique_ptr<capture_buffer_reader> create_multi_capture_buffer_reader(
    std::vector<std::unique_ptr<capture_buffer_file>>& buffers)
{
    std::vector<std::unique_ptr<capture_buffer_reader>> readers;
    std::transform(buffers.begin(),
                   buffers.end(),
                   std::back_inserter(readers),
                   [&](auto& buffer) {
                       return std::make_unique<capture_buffer_file_reader>(
                           *buffer);
                   });
    return std::unique_ptr<capture_buffer_reader>(
        new multi_capture_buffer_reader(std::move(readers)));
}

void fill_capture_buffers_ipv4(
    std::vector<std::unique_ptr<capture_buffer_file>>& buffers,
    size_t packet_count,
    size_t burst_size,
    size_t start_buffer = 0)
{
    mock_packet_buffer packet_buffer;
    std::array<uint8_t, 4096> packet_data;
    const uint32_t payload_size = 64;
    const auto num_buffers = buffers.size();

    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    create_ipv4_packet(packet_buffer,
                       packet_data.data(),
                       packet_data.size(),
                       payload_size,
                       now);

    std::array<struct packet_buffer*, 1> packet_buffers;
    packet_buffers[0] = reinterpret_cast<struct packet_buffer*>(&packet_buffer);

    for (size_t i = 0; i < packet_count; ++i) {
        packet_buffer.rx_timestamp = now + i;
        auto ipv4 = reinterpret_cast<ipv4_hdr*>(
            reinterpret_cast<uint8_t*>(packet_buffer.data) + sizeof(eth_hdr));
        ipv4->packet_id = ntohs(i);
        auto buffer_idx = ((i / burst_size) + start_buffer) % num_buffers;
        buffers[buffer_idx]->push(packet_buffers.data(), 1);
    }
}

size_t
verify_ipv4_incrementing_timestamp_and_packet_id(capture_buffer_reader& reader)
{
    size_t counted = 0;
    uint16_t prev_packet_id = 0;
    uint64_t prev_timestamp = 0;

    while (!reader.is_done()) {
        auto& packet = reader.get();
        REQUIRE(packet.packet_len != 0);
        REQUIRE(packet.captured_len <= packet.packet_len);
        auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet.data + sizeof(eth_hdr));
        if (counted > 0) {
            REQUIRE(packet.timestamp > prev_timestamp);
            REQUIRE(ntohs(ipv4->packet_id) == (prev_packet_id + 1));
        }
        prev_timestamp = packet.timestamp;
        prev_packet_id = ntohs(ipv4->packet_id);
        ++counted;
        reader.next();
    }
    return counted;
}

TEST_CASE("capture buffer", "[packet_capture]")
{
    SECTION("file, ")
    {
        const char* capture_filename = "unit_test.pcapng";

        SECTION("create, ")
        {
            {
                capture_buffer_file buffer(capture_filename);
                REQUIRE(std::filesystem::exists(capture_filename));
            }
            std::remove(capture_filename);
            REQUIRE(!std::filesystem::exists(capture_filename));
        }

        SECTION("write and read, ")
        {
            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            const uint32_t payload_size = 64;

            auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

            create_ipv4_packet(packet_buffer,
                               packet_data.data(),
                               packet_data.size(),
                               payload_size,
                               now);

            std::array<struct packet_buffer*, 1> packet_buffers;
            packet_buffers[0] =
                reinterpret_cast<struct packet_buffer*>(&packet_buffer);

            const int packet_count = 100;
            capture_buffer_file buffer(capture_filename);
            REQUIRE(std::filesystem::exists(capture_filename));
            for (int i = 0; i < packet_count; ++i) {
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_buffers.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                buffer.push(packet_buffers.data(), 1);
            }

            SECTION("iterator, ")
            {
                int counted = 0;
                for (auto& p : buffer) {
                    REQUIRE(p.packet_len == packet_buffer.length);
                    REQUIRE(p.captured_len == packet_buffer.length);
                    ++counted;
                }
                REQUIRE(counted == packet_count);
            }

            SECTION("reader, ")
            {
                int counted = 0;
                capture_buffer_file_reader reader(buffer);
                while (!reader.is_done()) {
                    auto& p = reader.get();
                    REQUIRE(p.packet_len == packet_buffer.length);
                    REQUIRE(p.captured_len == packet_buffer.length);
                    ++counted;
                    reader.next();
                }
                REQUIRE(counted == packet_count);
            }
        }
    }

    SECTION("multi buffer reader ")
    {
        SECTION("write and read, ")
        {
            SECTION("interleaved, burst size 1")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer_file>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = 1;
                fill_capture_buffers_ipv4(buffers, packet_count, burst_size);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == packet_count);
            }

            SECTION("interleaved, burst size 2")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer_file>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = 2;
                fill_capture_buffers_ipv4(buffers, packet_count, burst_size);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == packet_count);
            }

            SECTION("interleaved, burst size 4, starting w/ last buffer")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer_file>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = 4;
                fill_capture_buffers_ipv4(
                    buffers, packet_count, burst_size, num_buffers - 1);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == packet_count);
            }

            SECTION("no overlap")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer_file>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = packet_count / num_buffers;
                fill_capture_buffers_ipv4(buffers, packet_count, burst_size);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == packet_count);
            }
        }
    }
}
