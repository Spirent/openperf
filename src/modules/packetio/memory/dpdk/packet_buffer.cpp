#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/packet_buffer.hpp"

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

packet_type::flags type(const packet_buffer* buffer)
{
    return (packet_type::flags(buffer->packet_type));
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
        ol_flags |= (PKT_TX_IP_CKSUM | PKT_TX_IPV4);
    }
    if (flags & packet_type::ip::ipv6) { ol_flags |= PKT_TX_IPV6; }
    if (flags & packet_type::protocol::udp) { ol_flags |= PKT_TX_UDP_CKSUM; }
    if (flags & packet_type::protocol::tcp) { ol_flags |= PKT_TX_TCP_CKSUM; }

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

void type(packet_buffer* buffer, packet_type::flags flags)
{
    buffer->packet_type = flags.value;
}

uint16_t length(const packet_buffer* buffer)
{
    return (rte_pktmbuf_pkt_len(buffer));
}

timesync::chrono::realtime::time_point rx_timestamp(const packet_buffer* buffer)
{
    using clock = openperf::timesync::chrono::realtime;
    return (clock::time_point{clock::duration{buffer->timestamp}});
}

uint32_t rss_hash(const packet_buffer* buffer) { return (buffer->hash.rss); }

std::optional<uint32_t> signature_stream_id(const packet_buffer* buffer)
{
    return (dpdk::mbuf_signature_avail(buffer)
                ? std::make_optional(dpdk::mbuf_signature_stream_id_get(buffer))
                : std::nullopt);
}

std::optional<uint32_t> signature_sequence_number(const packet_buffer* buffer)
{
    return (dpdk::mbuf_signature_avail(buffer)
                ? std::make_optional(dpdk::mbuf_signature_seq_num_get(buffer))
                : std::nullopt);
}

std::optional<timesync::chrono::realtime::time_point>
signature_tx_timestamp(const packet_buffer* buffer)
{
    using clock = openperf::timesync::chrono::realtime;
    return (dpdk::mbuf_signature_avail(buffer)
                ? std::make_optional(clock::time_point{clock::duration{
                    dpdk::mbuf_signature_timestamp_get(buffer)}})
                : std::nullopt);
}

} // namespace openperf::packetio::packet
