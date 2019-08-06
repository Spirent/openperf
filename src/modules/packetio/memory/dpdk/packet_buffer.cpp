#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/packet_buffer.h"

namespace icp::packetio::packets {

/*
 * For our DPDK backend, the packet_buffer is a rte_mbuf.
 * However, we don't want to expose that fact to our clients.
 * This empty struct effectively just gives rte_mbuf a new type.
 */
struct packet_buffer : public rte_mbuf {};

uint16_t max_length(const packet_buffer* buffer)
{
    return (rte_pktmbuf_data_len(buffer));
}

uint16_t length(const packet_buffer* buffer)
{
    return (rte_pktmbuf_pkt_len(buffer));
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

}
