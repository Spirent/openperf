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
            std::array<uint8_t, 4> src_ip{1, 0, 0, 1};
            std::array<uint8_t, 4> dst_ip{1, 0, 0, 2};
            std::array<uint8_t, 6> src_mac{0x10, 0, 0, 0, 0, 1};
            std::array<uint8_t, 6> dst_mac{0x10, 0, 0, 0, 0, 2};
            const uint32_t payload_size = 64;

            mock_packet_buffer packet_buffer;
            std::array<uint8_t, 4096> packet_data;
            packet_buffer.data = packet_data.data();
            packet_buffer.data_length = packet_data.size();
            packet_buffer.rx_timestamp =
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::system_clock::now().time_since_epoch())
                    .count();

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

            std::array<struct packet_buffer*, 1> packet_buffers;
            packet_buffers[0] =
                reinterpret_cast<struct packet_buffer*>(&packet_buffer);

            const int packet_count = 100;
            capture_buffer_file buffer(capture_filename);
            REQUIRE(std::filesystem::exists(capture_filename));
            for (int i = 0; i < packet_count; ++i) {
                ipv4->packet_id = ntohs(i);
                buffer.push(packet_buffers.data(), 1);
            }

            int counted = 0;
            for (auto& pb : buffer) {
                ++counted;
                REQUIRE(pb.packet_len == packet_buffer.length);
            }
            REQUIRE(counted == packet_count);
        }

        SECTION("read", "") {}
    }
}
