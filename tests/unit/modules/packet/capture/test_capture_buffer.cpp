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
    const std::array<uint8_t, 4> src_ip{1, 0, 0, 1};
    const std::array<uint8_t, 4> dst_ip{1, 0, 0, 2};
    const std::array<uint8_t, 6> src_mac{0x10, 0, 0, 0, 0, 1};
    const std::array<uint8_t, 6> dst_mac{0x10, 0, 0, 0, 0, 2};

    assert(sizeof(eth_hdr) + sizeof(ipv4_hdr) + FCS_SIZE + payload_size
           <= data_size);

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
    std::vector<std::unique_ptr<capture_buffer>>& buffers, size_t count)
{
    const size_t capture_buffer_size = 1 * 1024 * 1024;
    buffers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        buffers.emplace_back(std::unique_ptr<capture_buffer>(
            new capture_buffer_mem(capture_buffer_size)));
    }
}

std::unique_ptr<capture_buffer_reader> create_multi_capture_buffer_reader(
    std::vector<std::unique_ptr<capture_buffer>>& buffers)
{
    std::vector<std::unique_ptr<capture_buffer_reader>> readers;
    std::transform(buffers.begin(),
                   buffers.end(),
                   std::back_inserter(readers),
                   [&](auto& buffer) { return buffer->create_reader(); });
    return std::unique_ptr<capture_buffer_reader>(
        new multi_capture_buffer_reader(std::move(readers)));
}

void fill_capture_buffers_ipv4(
    std::vector<std::unique_ptr<capture_buffer>>& buffers,
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
        buffers[buffer_idx]->write_packets(packet_buffers.data(), 1);
    }
}

size_t
verify_ipv4_incrementing_timestamp_and_packet_id(capture_buffer_reader& reader)
{
    size_t counted = 0;
    uint16_t prev_packet_id = 0;
    uint64_t prev_timestamp = 0;
    std::array<capture_packet*, 16> packets;

    while (!reader.is_done()) {
        auto n = reader.read_packets(packets.data(), packets.size());
        std::for_each(packets.data(), packets.data() + n, [&](auto packet) {
            REQUIRE(packet->hdr.packet_len != 0);
            REQUIRE(packet->hdr.captured_len <= packet->hdr.packet_len);
            auto ipv4 =
                reinterpret_cast<ipv4_hdr*>(packet->data + sizeof(eth_hdr));
            if (counted > 0) {
                REQUIRE(packet->hdr.timestamp > prev_timestamp);
                REQUIRE(ntohs(ipv4->packet_id) == (prev_packet_id + 1));
            }
            prev_timestamp = packet->hdr.timestamp;
            prev_packet_id = ntohs(ipv4->packet_id);
            ++counted;
        });
    }
    return counted;
}

size_t verify_ipv4_incrementing_timestamp_and_packet_id_iterator(
    capture_buffer_reader& reader)
{
    size_t counted = 0;
    uint16_t prev_packet_id = 0;
    uint64_t prev_timestamp = 0;

    for (auto& packet : reader) {
        REQUIRE(packet.hdr.packet_len != 0);
        REQUIRE(packet.hdr.captured_len <= packet.hdr.packet_len);
        auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet.data + sizeof(eth_hdr));
        if (counted > 0) {
            REQUIRE(packet.hdr.timestamp > prev_timestamp);
            REQUIRE(ntohs(ipv4->packet_id) == (prev_packet_id + 1));
        }
        prev_timestamp = packet.hdr.timestamp;
        prev_packet_id = ntohs(ipv4->packet_id);
        ++counted;
    }
    return counted;
}

TEST_CASE("capture buffer", "[packet_capture]")
{
    SECTION("mem, ")
    {
        SECTION("create, ")
        {
            SECTION("success")
            {
                capture_buffer_mem buffer(1 * 1024 * 1024);
                auto stats = buffer.get_stats();
                REQUIRE(stats.packets == 0);
                REQUIRE(stats.bytes == 0);
            }
            SECTION("failure, too large")
            {
                // Try to allocate 1 TB of memory
                REQUIRE_THROWS_AS(
                    capture_buffer_mem(1024ULL * 1024 * 1024 * 1024),
                    std::bad_alloc);
            }
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
            capture_buffer_mem buffer(1 * 1024 * 1024);
            for (int i = 0; i < packet_count; ++i) {
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_data.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                buffer.write_packets(packet_buffers.data(), 1);
            }

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * packet_buffer.length);

            SECTION("iterator, ")
            {
                int counted = 0;
                for (auto& packet : buffer) {
                    REQUIRE(packet.hdr.packet_len == packet_buffer.length);
                    REQUIRE(packet.hdr.captured_len == packet_buffer.length);
                    ++counted;
                }
                REQUIRE(counted == packet_count);
            }

            SECTION("reader, ")
            {
                int counted = 0;
                auto reader = buffer.create_reader();
                std::array<capture_packet*, 16> packets;
                while (!reader->is_done()) {
                    auto n =
                        reader->read_packets(packets.data(), packets.size());
                    std::for_each(
                        packets.data(), packets.data() + n, [&](auto& packet) {
                            REQUIRE(packet->hdr.packet_len
                                    == packet_buffer.length);
                            REQUIRE(packet->hdr.captured_len
                                    == packet_buffer.length);
                            ++counted;
                        });
                }
                REQUIRE(counted == packet_count);
            }
        }

        SECTION("write and read, trucated packets")
        {
            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            const uint32_t payload_size = packet_data.size() - sizeof(eth_hdr)
                                          - sizeof(ipv4_hdr) - FCS_SIZE;
            const uint32_t capture_max_packet_size = 1500;

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
            capture_buffer_mem buffer(1 * 1024 * 1024, capture_max_packet_size);

            for (int i = 0; i < packet_count; ++i) {
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_data.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                buffer.write_packets(packet_buffers.data(), 1);
            }

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * capture_max_packet_size);

            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_buffer.length);
                REQUIRE(packet.hdr.captured_len == capture_max_packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("write and read, trucated packets")
        {
            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            const uint32_t payload_size = packet_data.size() - sizeof(eth_hdr)
                                          - sizeof(ipv4_hdr) - FCS_SIZE;
            const uint32_t capture_max_packet_size = 1500;

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
            capture_buffer_mem buffer(1 * 1024 * 1024, capture_max_packet_size);

            for (int i = 0; i < packet_count; ++i) {
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_data.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                buffer.write_packets(packet_buffers.data(), 1);
            }

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * capture_max_packet_size);

            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_buffer.length);
                REQUIRE(packet.hdr.captured_len == capture_max_packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("write and read to full buffer")
        {
            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            const uint32_t payload_size = packet_data.size() - sizeof(eth_hdr)
                                          - sizeof(ipv4_hdr) - FCS_SIZE;

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

            capture_buffer_mem buffer(1 * 1024 * 1024);

            // Write packets until buffer gets full
            uint32_t i = 0;
            while (true) {
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_data.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                if (buffer.write_packets(packet_buffers.data(), 1) < 0) break;
                ++i;
            }
            REQUIRE(buffer.is_full());
            auto packet_count = i;

            // Buffer should be full so write should fail
            REQUIRE(buffer.write_packets(packet_buffers.data(), 1) < 0);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * packet_data.size());

            // Validate packets in buffer
            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_buffer.length);
                REQUIRE(packet.hdr.captured_len == packet_data.size());
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }
    }

    SECTION("file, ")
    {
        const char* capture_filename = "unit_test.pcapng";

        SECTION("create, ")
        {
            SECTION("success")
            {
                {
                    capture_buffer_file buffer(capture_filename);
                    REQUIRE(std::filesystem::exists(capture_filename));
                }
                std::remove(capture_filename);
                REQUIRE(!std::filesystem::exists(capture_filename));
            }

            SECTION("failure, bad path")
            {
                REQUIRE_THROWS_AS(
                    capture_buffer_file("/tmp/bad_path_to_file/file.pcapng"),
                    std::runtime_error);
            }
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
                auto ipv4 = reinterpret_cast<ipv4_hdr*>(packet_data.data()
                                                        + sizeof(eth_hdr));
                ipv4->packet_id = ntohs(i);
                buffer.write_packets(packet_buffers.data(), 1);
            }

            SECTION("iterator, ")
            {
                int counted = 0;
                for (auto& packet : buffer) {
                    REQUIRE(packet.hdr.packet_len == packet_buffer.length);
                    REQUIRE(packet.hdr.captured_len == packet_buffer.length);
                    ++counted;
                }
                REQUIRE(counted == packet_count);
            }

            SECTION("reader, ")
            {
                int counted = 0;
                auto reader = buffer.create_reader();
                std::array<capture_packet*, 16> packets;
                while (!reader->is_done()) {
                    auto n =
                        reader->read_packets(packets.data(), packets.size());
                    std::for_each(
                        packets.data(), packets.data() + n, [&](auto& packet) {
                            REQUIRE(packet->hdr.packet_len
                                    == packet_buffer.length);
                            REQUIRE(packet->hdr.captured_len
                                    == packet_buffer.length);
                            ++counted;
                        });
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
                std::vector<std::unique_ptr<capture_buffer>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = 1;
                fill_capture_buffers_ipv4(buffers, packet_count, burst_size);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == packet_count);
            }

            SECTION("interleaved, burst size 1, iterator")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer>> buffers;
                create_capture_buffers(buffers, num_buffers);

                const size_t packet_count = 1000;
                const size_t burst_size = 1;
                fill_capture_buffers_ipv4(buffers, packet_count, burst_size);

                auto reader = create_multi_capture_buffer_reader(buffers);
                auto counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id_iterator(
                        *reader);
                REQUIRE(counted == packet_count);
            }

            SECTION("interleaved, burst size 2")
            {
                const size_t num_buffers = 5;
                std::vector<std::unique_ptr<capture_buffer>> buffers;
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
                std::vector<std::unique_ptr<capture_buffer>> buffers;
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
                std::vector<std::unique_ptr<capture_buffer>> buffers;
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
