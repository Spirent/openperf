#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/mbuf_signature.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packetio::packets {

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

uint32_t type(const packet_buffer* buffer) { return (buffer->packet_type); }

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

void length(packet_buffer* buffer, uint16_t size)
{
    rte_pktmbuf_data_len(buffer) = size;
    rte_pktmbuf_pkt_len(buffer) = size;
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

} // namespace openperf::packetio::packets
