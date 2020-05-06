#include <chrono>
#include <filesystem>
#include <random>
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

static size_t calc_ipv4_packet_size(size_t payload_size)
{
    const size_t encap_size = sizeof(eth_hdr) + sizeof(ipv4_hdr) + FCS_SIZE;
    return (encap_size + payload_size);
}

static size_t calc_ipv4_payload_size(size_t packet_size)
{
    const size_t encap_size = sizeof(eth_hdr) + sizeof(ipv4_hdr) + FCS_SIZE;
    assert(packet_size >= encap_size);
    return (packet_size - encap_size);
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

    assert(calc_ipv4_packet_size(payload_size) <= data_size);

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

size_t fill_capture_buffer_ipv4(capture_buffer& buffer,
                                size_t packet_count,
                                size_t payload_size,
                                uint16_t start_id = 0)
{
    mock_packet_buffer packet_buffer;
    std::vector<uint8_t> packet_data;

    packet_data.resize(calc_ipv4_packet_size(payload_size));

    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    create_ipv4_packet(
        packet_buffer, &packet_data[0], packet_data.size(), payload_size, now);

    std::array<struct packet_buffer*, 1> packet_buffers;
    packet_buffers[0] = reinterpret_cast<struct packet_buffer*>(&packet_buffer);

    for (size_t i = 0; i < packet_count; ++i) {
        packet_buffer.rx_timestamp = now + i;
        auto ipv4 = reinterpret_cast<ipv4_hdr*>(
            reinterpret_cast<uint8_t*>(packet_buffer.data) + sizeof(eth_hdr));
        ipv4->packet_id = ntohs(start_id + i);
        if (buffer.write_packets(packet_buffers.data(), 1) != 1) { return i; }
    }

    return packet_count;
}

size_t
fill_capture_buffers_ipv4(std::vector<std::unique_ptr<capture_buffer>>& buffers,
                          size_t packet_count,
                          size_t burst_size,
                          size_t start_buffer = 0,
                          size_t payload_size = 64)
{
    mock_packet_buffer packet_buffer;
    std::vector<uint8_t> packet_data;
    const auto num_buffers = buffers.size();

    packet_data.resize(calc_ipv4_packet_size(payload_size));

    auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();

    create_ipv4_packet(
        packet_buffer, &packet_data[0], packet_data.size(), payload_size, now);

    std::array<struct packet_buffer*, 1> packet_buffers;
    packet_buffers[0] = reinterpret_cast<struct packet_buffer*>(&packet_buffer);

    for (size_t i = 0; i < packet_count; ++i) {
        packet_buffer.rx_timestamp = now + i;
        auto ipv4 = reinterpret_cast<ipv4_hdr*>(
            reinterpret_cast<uint8_t*>(packet_buffer.data) + sizeof(eth_hdr));
        ipv4->packet_id = ntohs(i);
        auto buffer_idx = ((i / burst_size) + start_buffer) % num_buffers;
        if (buffers[buffer_idx]->write_packets(packet_buffers.data(), 1) != 1)
            return i;
    }
    return packet_count;
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
            INFO("Packet offset " << counted);
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
        INFO("Packet offset " << counted);
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
            const size_t packet_count = 100;
            const size_t payload_size = 64;
            const size_t packet_size = calc_ipv4_packet_size(64);

            capture_buffer_mem buffer(1 * 1024 * 1024);
            fill_capture_buffer_ipv4(buffer, packet_count, payload_size);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * packet_size);

            SECTION("iterator, ")
            {
                int counted = 0;
                for (auto& packet : buffer) {
                    REQUIRE(packet.hdr.packet_len == packet_size);
                    REQUIRE(packet.hdr.captured_len == packet_size);
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
                            REQUIRE(packet->hdr.packet_len == packet_size);
                            REQUIRE(packet->hdr.captured_len == packet_size);
                            ++counted;
                        });
                }
                REQUIRE(counted == packet_count);
            }
        }

        SECTION("write and read, trucated packets")
        {
            const uint32_t capture_max_packet_size = 1500;
            const size_t packet_size = 4096;
            const size_t payload_size = calc_ipv4_payload_size(packet_size);
            const size_t packet_count = 100;

            capture_buffer_mem buffer(1 * 1024 * 1024, capture_max_packet_size);

            fill_capture_buffer_ipv4(buffer, packet_count, payload_size);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * capture_max_packet_size);

            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_size);
                REQUIRE(packet.hdr.captured_len == capture_max_packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("write and read to full buffer")
        {
            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            const size_t packet_size = packet_data.size();
            const size_t payload_size = calc_ipv4_payload_size(packet_size);

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
                if (buffer.write_packets(packet_buffers.data(), 1) != 1) break;
                ++i;
            }
            REQUIRE(buffer.is_full());
            auto packet_count = i;

            // Buffer should be full so write should fail
            REQUIRE(buffer.write_packets(packet_buffers.data(), 1) != 1);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * packet_size);

            // Validate packets in buffer
            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_size);
                REQUIRE(packet.hdr.captured_len == packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }
    }

    SECTION("mem wrap, ")
    {
        SECTION("create, ")
        {
            SECTION("success")
            {
                capture_buffer_mem_wrap buffer(1 * 1024 * 1024);
                auto stats = buffer.get_stats();
                REQUIRE(stats.packets == 0);
                REQUIRE(stats.bytes == 0);
            }
            SECTION("failure, too large")
            {
                // Try to allocate 1 TB of memory
                REQUIRE_THROWS_AS(
                    capture_buffer_mem_wrap(1024ULL * 1024 * 1024 * 1024),
                    std::bad_alloc);
            }
        }

        SECTION("write and read, ")
        {
            const size_t packet_count = 100;
            const size_t payload_size = 64;
            const size_t packet_size = calc_ipv4_packet_size(64);

            capture_buffer_mem_wrap buffer(1 * 1024 * 1024);
            fill_capture_buffer_ipv4(buffer, packet_count, payload_size);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * packet_size);

            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_size);
                REQUIRE(packet.hdr.captured_len == packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("write and read, trucated packets")
        {
            const size_t capture_max_packet_size = 1500;
            const size_t packet_size = 4096;
            const size_t payload_size = calc_ipv4_payload_size(packet_size);
            const size_t packet_count = 100;

            capture_buffer_mem_wrap buffer(1 * 1024 * 1024,
                                           capture_max_packet_size);

            fill_capture_buffer_ipv4(buffer, packet_count, payload_size);

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == packet_count);
            REQUIRE(stats.bytes == packet_count * capture_max_packet_size);

            int counted = 0;
            for (auto& packet : buffer) {
                REQUIRE(packet.hdr.packet_len == packet_size);
                REQUIRE(packet.hdr.captured_len == capture_max_packet_size);
                ++counted;
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("write and read with wrap, same size")
        {
            const uint64_t buffer_size = 1 * 1024 * 1024;
            const size_t packet_size = 4096;
            const size_t payload_size = calc_ipv4_payload_size(packet_size);
            const size_t buffer_bytes_per_packet =
                (sizeof(capture_packet_hdr)
                 + pad_capture_data_len(packet_size));
            const size_t max_packets_in_buffer =
                (buffer_size / buffer_bytes_per_packet);
            size_t total_written = 0, nwritten = 0;

            capture_buffer_mem_wrap buffer(buffer_size);

            // Write packets until buffer is about to wrap
            nwritten = fill_capture_buffer_ipv4(
                buffer, max_packets_in_buffer, payload_size);
            REQUIRE(nwritten == max_packets_in_buffer);
            REQUIRE(buffer.is_full() == false);
            REQUIRE(buffer.get_wrap_addr() == nullptr);
            REQUIRE(buffer.get_wrap_end_addr() == nullptr);
            total_written += nwritten;

            // Validate buffer stats
            auto stats = buffer.get_stats();
            REQUIRE(stats.packets == max_packets_in_buffer);
            REQUIRE(stats.bytes == max_packets_in_buffer * packet_size);

            // Validate packets in buffer
            size_t counted;
            auto reader = buffer.create_reader();
            counted = verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
            REQUIRE(counted == stats.packets);

            // Write another packet
            nwritten = fill_capture_buffer_ipv4(
                buffer, 1, payload_size, uint16_t(total_written));
            REQUIRE(nwritten == 1);
            REQUIRE(buffer.is_full() == false);
            REQUIRE(buffer.get_wrap_addr() != nullptr);
            REQUIRE(buffer.get_wrap_end_addr() != nullptr);
            total_written += nwritten;

            // Validate buffer stats
            // Note: This is only true because all packets are the same size
            stats = buffer.get_stats();
            REQUIRE(stats.packets == max_packets_in_buffer);
            REQUIRE(stats.bytes == max_packets_in_buffer * packet_size);

            // Validate packets in buffer
            reader->rewind();
            counted = verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
            REQUIRE(counted == stats.packets);

            // Write another bunch of packet
            nwritten = fill_capture_buffer_ipv4(buffer,
                                                max_packets_in_buffer / 2,
                                                payload_size,
                                                uint16_t(total_written));
            REQUIRE(nwritten == max_packets_in_buffer / 2);
            REQUIRE(buffer.is_full() == false);
            REQUIRE(buffer.get_wrap_addr() != nullptr);
            REQUIRE(buffer.get_wrap_end_addr() != nullptr);
            total_written += nwritten;

            // Validate buffer stats
            // Note: This is only true because all packets are the same size
            stats = buffer.get_stats();
            REQUIRE(stats.packets == max_packets_in_buffer);
            REQUIRE(stats.bytes == max_packets_in_buffer * packet_size);

            // Validate packets in buffer
            reader->rewind();
            counted = verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
            REQUIRE(counted == stats.packets);
        }

        SECTION("write and read with wrap, double size")
        {
            const uint64_t buffer_size = 1 * 1024 * 1024;
            size_t total_written = 0, nwritten = 0;

            capture_buffer_mem_wrap buffer(buffer_size);

            for (int size_multiplier = 1; size_multiplier <= 4;
                 size_multiplier *= 2) {
                const size_t packet_size = 64 * size_multiplier;
                const size_t payload_size = calc_ipv4_payload_size(packet_size);
                const size_t buffer_bytes_per_packet =
                    (sizeof(capture_packet_hdr)
                     + pad_capture_data_len(packet_size));
                const size_t max_packets_in_buffer =
                    (buffer_size / buffer_bytes_per_packet);

                INFO("packet_size " << packet_size);
                INFO("max_packets_in_buffer " << max_packets_in_buffer);

                // Fill buffer 2x with increasing packet sizes
                // This tests that the buffer wraps correctly and handles
                // reclaiming space from multiple packets
                nwritten = fill_capture_buffer_ipv4(buffer,
                                                    2 * max_packets_in_buffer,
                                                    payload_size,
                                                    uint16_t(total_written));
                REQUIRE(nwritten == 2 * max_packets_in_buffer);
                total_written += nwritten;

                // The assumption is that this should always be at the end of
                // the buffer.  When not at end of buffer could have 1 less
                // packet than expected due to wrap case
                REQUIRE(buffer.get_write_addr() + buffer_bytes_per_packet
                        >= buffer.get_end_addr());

                // Validate buffer stats
                auto stats = buffer.get_stats();
                REQUIRE(stats.packets == max_packets_in_buffer);
                REQUIRE(stats.bytes == max_packets_in_buffer * packet_size);

                // Validate packets in buffer
                size_t counted;
                auto reader = buffer.create_reader();
                counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == stats.packets);
            }
        }

        SECTION("write and read with wrap, random size")
        {
            const uint64_t buffer_size = 1 * 1024 * 1024;
            const size_t min_buffer_bytes_per_packet =
                (sizeof(capture_packet_hdr)
                 + pad_capture_data_len(calc_ipv4_packet_size(0)));
            const size_t max_packets_in_buffer =
                (buffer_size / min_buffer_bytes_per_packet);
            size_t total_written = 0, nwritten = 0;

            capture_buffer_mem_wrap buffer(buffer_size);

            // Want same random number sequence all the time for debugability
            auto seed = 1;
            std::default_random_engine gen(seed);

            // Use random packet sizes over a few different ranges so that
            // the number of packets in buffer has higher variability
            std::pair<size_t, size_t> ranges[] = {
                {0, 64},
                {64, 256},
                {256, 1024},
                {1024, 1500},
                {0, 4000},
            };

            for (auto& range : ranges) {
                INFO("range " << range.first << " to " << range.second);
                std::uniform_int_distribution<size_t> dist(range.first,
                                                           range.second);
                for (size_t i = 0; i < 2 * max_packets_in_buffer; ++i) {
                    const size_t payload_size = dist(gen);
                    nwritten = fill_capture_buffer_ipv4(
                        buffer, 1, payload_size, uint16_t(total_written));
                    REQUIRE(nwritten == 1);
                    total_written += nwritten;
                }

                // Since packet sizes are randmon, don't really know what is
                // currently in the buffer so can't do too much additional
                // validation checks

                // Validate buffer stats
                auto stats = buffer.get_stats();
                REQUIRE(stats.packets <= max_packets_in_buffer);

                // Validate packets in buffer
                size_t counted;
                auto reader = buffer.create_reader();
                counted =
                    verify_ipv4_incrementing_timestamp_and_packet_id(*reader);
                REQUIRE(counted == stats.packets);
            }
        }
    }

    SECTION("file, ")
    {
        const char* capture_filename = "unit_test.pcapng";

        SECTION("create, ")
        {
            SECTION("success, no keep")
            {
                {
                    capture_buffer_file buffer(capture_filename);
                    REQUIRE(std::filesystem::exists(capture_filename));
                }
                REQUIRE(!std::filesystem::exists(capture_filename));
            }

            SECTION("success, keep")
            {
                {
                    capture_buffer_file buffer(capture_filename, true);
                    REQUIRE(std::filesystem::exists(capture_filename));
                }
                REQUIRE(std::filesystem::exists(capture_filename));
                std::remove(capture_filename);
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
            const size_t packet_count = 100;
            const size_t payload_size = 64;
            const size_t packet_size = calc_ipv4_packet_size(64);

            capture_buffer_file buffer(capture_filename);
            REQUIRE(std::filesystem::exists(capture_filename));

            fill_capture_buffer_ipv4(buffer, packet_count, payload_size);

            SECTION("iterator, ")
            {
                int counted = 0;
                for (auto& packet : buffer) {
                    REQUIRE(packet.hdr.packet_len == packet_size);
                    REQUIRE(packet.hdr.captured_len == packet_size);
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
                            REQUIRE(packet->hdr.packet_len == packet_size);
                            REQUIRE(packet->hdr.captured_len == packet_size);
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
