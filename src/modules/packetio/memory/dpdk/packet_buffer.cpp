#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_metadata.hpp"
#include "packetio/packet_buffer.hpp"
#include "spirent_pga/api.h"

namespace openperf::packetio::packet {

/*
 * For our DPDK backend, the packet_buffer is a rte_mbuf.
 * However, we don't want to expose that fact to our clients.
 * This empty struct effectively just gives rte_mbuf a new type.
 */
struct packet_buffer : public rte_mbuf
{};

uint16_t max_length(const packet_buffer* buffer)
{
    return (rte_pktmbuf_data_len(buffer));
}

void* to_data(packet_buffer* buffer)
{
    return (rte_pktmbuf_mtod(buffer, void*));
}

const void* to_data(const packet_buffer* buffer)
{
    return (rte_pktmbuf_mtod(buffer, void*));
}

void* to_data(packet_buffer* buffer, uint16_t offset)
{
    return (rte_pktmbuf_mtod_offset(buffer, void*, offset));
}

const void* to_data(const packet_buffer* buffer, uint16_t offset)
{
    return (rte_pktmbuf_mtod_offset(buffer, void*, offset));
}

void* prepend(packet_buffer* buffer, uint16_t offset)
{
    return (rte_pktmbuf_prepend(buffer, offset));
}

void* append(packet_buffer* buffer, uint16_t offset)
{
    return (rte_pktmbuf_append(buffer, offset));
}

void* front(packet_buffer* buffer)
{
    return (rte_pktmbuf_prepend(buffer, buffer->data_off));
}

void length(packet_buffer* buffer, uint16_t size)
{
    rte_pktmbuf_data_len(buffer) = size;
    rte_pktmbuf_pkt_len(buffer) = size;
}

void tx_offload(packet_buffer* buffer,
                header_lengths hdr_lens,
                packet_type::flags flags)
{
    /*
     * Our header lengths structure contains additional data compared
     * to the equivalent DPDK structure. Generate an explicit mask so that we
     * can clear any bits that shouldn't be set.
     */
    constexpr static auto mask = header_lengths{
        .layer2 = 0x7f,
        .layer3 = 0x1ff,
        .layer4 = 0xff,
        .tso_segsz = 0xffff,
    };

    uint64_t ol_flags = 0;
    if (flags & packet_type::ip::ipv4) {
        ol_flags |= (RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4);
    }
    if (flags & packet_type::ip::ipv6) { ol_flags |= RTE_MBUF_F_TX_IPV6; }
    if (flags & packet_type::protocol::udp) {
        ol_flags |= RTE_MBUF_F_TX_UDP_CKSUM;
    }
    if (flags & packet_type::protocol::tcp) {
        ol_flags |= RTE_MBUF_F_TX_TCP_CKSUM;
    }

    /* Update packet metadata */
    buffer->ol_flags = ol_flags;
    buffer->tx_offload = hdr_lens.value & mask.value;
}

void signature(packet_buffer* buffer,
               uint32_t stream_id,
               uint32_t seq_num,
               int flags)
{
    dpdk::mbuf_signature_tx_set(buffer, stream_id, seq_num, flags);
}

void signature_fill_const(packet_buffer* buffer,
                          uint16_t offset,
                          uint16_t value)
{
    dpdk::mbuf_signature_tx_set_fill_const(buffer, offset, value);
}

void signature_fill_decr(packet_buffer* buffer, uint16_t offset, uint8_t value)
{
    dpdk::mbuf_signature_tx_set_fill_decr(buffer, offset, value);
}

void signature_fill_incr(packet_buffer* buffer, uint16_t offset, uint8_t value)
{
    dpdk::mbuf_signature_tx_set_fill_incr(buffer, offset, value);
}

void signature_fill_prbs(packet_buffer* buffer, uint16_t offset)
{
    dpdk::mbuf_signature_tx_set_fill_prbs(buffer, offset);
}

void packet_type_flags(packet_buffer* buffer, packet_type::flags flags)
{
    buffer->packet_type = flags.value;
}

uint16_t length(const packet_buffer* buffer)
{
    return (rte_pktmbuf_pkt_len(buffer));
}

uint16_t frame_length(const packet_buffer* buffer)
{
    /* I guess this is true for every driver? ¯\_(ツ)_/¯ */
    return (rte_pktmbuf_pkt_len(buffer) + RTE_ETHER_CRC_LEN);
}

timesync::chrono::realtime::time_point rx_timestamp(const packet_buffer* buffer)
{
    using clock = openperf::timesync::chrono::realtime;
    return (clock::time_point{dpdk::mbuf_timestamp_get(buffer)});
}

packet_type::flags packet_type_flags(const packet_buffer* buffer)
{
    return (packet_type::flags(buffer->packet_type));
}

uint32_t rss_hash(const packet_buffer* buffer) { return (buffer->hash.rss); }

bool ipv4_checksum_error(const packet_buffer* buffer)
{
    return ((buffer->ol_flags & RTE_MBUF_F_RX_IP_CKSUM_MASK)
            == RTE_MBUF_F_RX_IP_CKSUM_BAD);
}

bool tcp_checksum_error(const packet_buffer* buffer)
{
    const auto is_tcp =
        packet_type::flags(buffer->packet_type) & packet_type::protocol::tcp;
    return (is_tcp.value
            && (buffer->ol_flags & RTE_MBUF_F_RX_L4_CKSUM_MASK)
                   == RTE_MBUF_F_RX_L4_CKSUM_BAD);
}

bool udp_checksum_error(const packet_buffer* buffer)
{
    const auto is_udp =
        packet_type::flags(buffer->packet_type) & packet_type::protocol::udp;
    return (is_udp.value
            && (buffer->ol_flags & RTE_MBUF_F_RX_L4_CKSUM_MASK)
                   == RTE_MBUF_F_RX_L4_CKSUM_BAD);
}

std::optional<uint32_t> prbs_octets(const packet_buffer* buffer)
{
    return (dpdk::mbuf_rx_prbs_is_set(buffer)
                ? std::make_optional(dpdk::mbuf_rx_prbs_get_octets(buffer))
                : std::nullopt);
}

std::optional<uint32_t> prbs_bit_errors(const packet_buffer* buffer)
{
    return (dpdk::mbuf_rx_prbs_is_set(buffer)
                ? std::make_optional(dpdk::mbuf_rx_prbs_get_bit_errors(buffer))
                : std::nullopt);
}

std::optional<uint32_t> signature_stream_id(const packet_buffer* buffer)
{
    return (dpdk::mbuf_signature_is_set(buffer)
                ? std::make_optional(dpdk::mbuf_signature_get_stream_id(buffer))
                : std::nullopt);
}

std::optional<uint32_t> signature_sequence_number(const packet_buffer* buffer)
{
    return (dpdk::mbuf_signature_is_set(buffer)
                ? std::make_optional(dpdk::mbuf_signature_get_seq_num(buffer))
                : std::nullopt);
}

std::optional<timesync::chrono::realtime::time_point>
signature_tx_timestamp(const packet_buffer* buffer)
{
    using clock = openperf::timesync::chrono::realtime;
    return (dpdk::mbuf_signature_is_set(buffer)
                ? std::make_optional(clock::time_point{clock::duration{
                    dpdk::mbuf_signature_get_timestamp(buffer)}})
                : std::nullopt);
}

bool tx_sink(const packet_buffer* buffer)
{
    return dpdk::mbuf_tx_sink_is_set(buffer);
}

} // namespace openperf::packetio::packet
