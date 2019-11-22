#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

#include <chrono>
#include <thread>

#include "zmq.h"

#include "core/op_core.h"
#include "core/op_uuid.h"
#include "net/net_types.h"
#include "units/rate.h"
#include "packetio/internal_client.h"
#include "packetio/packet_buffer.h"

#include "rte_ether.h"
#include "rte_ip.h"
#include "rte_udp.h"

static bool _op_done = false;
void signal_handler(int signo __attribute__((unused))) { _op_done = true; }

struct test_packet
{
    struct rte_ether_hdr ether;
    struct rte_ipv4_hdr ipv4;
    struct rte_udp_hdr udp;
} __attribute__((packed));

using namespace openperf::net;

static void initialize_eth_header(rte_ether_hdr& eth_hdr, mac_address& src_mac,
                                  mac_address& dst_mac, uint16_t ether_type)
{
    rte_ether_addr_copy(reinterpret_cast<const rte_ether_addr*>(dst_mac.data()),
                        &eth_hdr.d_addr);
    rte_ether_addr_copy(reinterpret_cast<const rte_ether_addr*>(src_mac.data()),
                        &eth_hdr.s_addr);
    eth_hdr.ether_type = htons(ether_type);
}

static void initialize_ipv4_header(rte_ipv4_hdr& ip_hdr, ipv4_address& src_addr,
                                   ipv4_address& dst_addr,
                                   uint16_t pkt_data_len)
{
    auto pkt_len = pkt_data_len + sizeof(ip_hdr);
    /*
     * Initialize IP header.
     */
    ip_hdr.version_ihl = 0x45;
    ip_hdr.type_of_service = 0;
    ip_hdr.fragment_offset = 0;
    ip_hdr.time_to_live = 0x10;
    ip_hdr.next_proto_id = IPPROTO_UDP;
    ip_hdr.hdr_checksum = 0;
    ip_hdr.packet_id = 0;
    ip_hdr.total_length = htons(pkt_len);
    ip_hdr.src_addr = htonl(src_addr.data());
    ip_hdr.dst_addr = htonl(dst_addr.data());

    /*
     * Compute IP header checksum.
     */
    auto ptr16 = reinterpret_cast<uint16_t*>(std::addressof(ip_hdr));
    unsigned ip_cksum =
        std::accumulate(ptr16, ptr16 + 10, 0U,
                        [](unsigned sum, auto chunk) { return (sum + chunk); });

    /*
     * Reduce 32 bit checksum to 16 bits and complement it.
     */
    ip_cksum = ((ip_cksum & 0xFFFF0000) >> 16) + (ip_cksum & 0x0000FFFF);
    ip_cksum = (~ip_cksum) & 0x0000FFFF;
    ip_hdr.hdr_checksum =
        static_cast<uint16_t>(ip_cksum == 0 ? 0xFFFF : ip_cksum);
}

static void initialize_udp_header(rte_udp_hdr& udp_hdr, uint16_t src_port,
                                  uint16_t dst_port, uint16_t pkt_data_len)
{
    auto pkt_len = pkt_data_len + sizeof(udp_hdr);
    udp_hdr.src_port = htons(src_port);
    udp_hdr.dst_port = htons(dst_port);
    udp_hdr.dgram_len = htons(pkt_len);
    udp_hdr.dgram_cksum = 0; /* No UDP checksum. */
}

class test_source
{
    using packets_per_second = openperf::units::rate<uint64_t>;
    using packets_per_minute =
        openperf::units::rate<uint64_t, std::ratio<1, 60>>;
    using packets_per_hour =
        openperf::units::rate<uint64_t, std::ratio<1, 3600>>;

    openperf::core::uuid m_id;
    packets_per_minute m_rate;

public:
    test_source(uint64_t ppm)
        : m_id(openperf::core::uuid::random())
        , m_rate(ppm)
    {}

    test_source(test_source&& other) = default;
    test_source& operator=(test_source&& other) = default;

    std::string id() const { return (openperf::core::to_string(m_id)); }

    packets_per_hour packet_rate() const { return (m_rate); }

    uint16_t transform(openperf::packetio::packets::packet_buffer* input[],
                       uint16_t input_length,
                       openperf::packetio::packets::packet_buffer* output[])
    {
        using namespace openperf::packetio;

        auto src_mac = mac_address{0x00, 0xff, 0xaa, 0xff, 0xaa, 0xff};
        auto dst_mac = mac_address{0x00, 0xaa, 0xff, 0xaa, 0xff, 0xaa};
        auto src_ip = ipv4_address("198.18.25.1");
        auto dst_ip = ipv4_address("198.19.25.1");

        std::for_each(input, input + input_length, [&](auto packet) {
            auto tmp = packets::to_data<test_packet>(packet);

            initialize_eth_header(tmp->ether, src_mac, dst_mac,
                                  RTE_ETHER_TYPE_IPV4);
            initialize_ipv4_header(tmp->ipv4, src_ip, dst_ip, 26);
            initialize_udp_header(tmp->udp, 3357, 3357, 18);

            packets::length(packet, 60);

            // rte_pktmbuf_dump(stderr, reinterpret_cast<rte_mbuf*>(packet),
            // 60);
            *output++ = packet;
        });
        return (input_length);
    }
};

void test_sources(void* context)
{
    using namespace std::chrono_literals;
    auto client = openperf::packetio::internal::api::client(context);

    auto source0 = openperf::packetio::packets::generic_source(
        test_source(30)); /* packet/2 sec */
    auto success = client.add_source("port0", source0);
    if (!success) {
        throw std::runtime_error("Could not add first source to port 0\n");
    }

    std::this_thread::sleep_for(11s);

    client.del_source("port0", source0);

    std::this_thread::sleep_for(5s);
}

int main(int argc, char* argv[])
{
    op_thread_setname("op_main");

    void* context = zmq_ctx_new();
    if (!context) { op_exit("Could not initialize ZeroMQ context!"); }

    /* Do initialization... */
    op_init(context, argc, argv);

    /* ... then do the sources thing... */
    test_sources(context);

    /* ... then clean up and exit. */
    op_halt(context);

    exit(EXIT_SUCCESS);
}
