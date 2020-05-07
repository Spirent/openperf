#ifndef _OP_PACKETIO_PACKET_BUFFER_HPP_
#define _OP_PACKETIO_PACKET_BUFFER_HPP_

#include <cstdint>
#include <cstddef>
#include <optional>

#include "packetio/packet_type.hpp"
#include "timesync/chrono.hpp"

namespace openperf::packetio::packet {

struct packet_buffer;

/**
 * Common functions
 */
uint16_t max_length(const packet_buffer* buffer);
packet_type::flags type(const packet_buffer* buffer);

void* to_data(packet_buffer* buffer);
const void* to_data(const packet_buffer* buffer);

void* to_data(packet_buffer* buffer, uint16_t offset);
const void* to_data(const packet_buffer* buffer, uint16_t offset);

void* prepend(packet_buffer* buffer, uint16_t offset);

void* append(packet_buffer* buffer, uint16_t offset);

void* front(packet_buffer* buffer);

/**
 * Templatized versions to convert pointers to specific types.
 */
template <typename T> T* to_data(packet_buffer* buffer)
{
    return (reinterpret_cast<T*>(to_data(buffer)));
}

template <typename T> const T* to_data(const packet_buffer* buffer)
{
    return (reinterpret_cast<T*>(to_data(buffer)));
}

template <typename T> T* to_data(packet_buffer* buffer, uint16_t offset)
{
    return (reinterpret_cast<T*>(to_data(buffer, offset)));
}

template <typename T>
const T* to_data(const packet_buffer* buffer, uint16_t offset)
{
    return (reinterpret_cast<T*>(to_data(buffer, offset)));
}

template <typename T> T* prepend(packet_buffer* buffer)
{
    return (reinterpret_cast<T*>(prepend(buffer, sizeof(T))));
}

template <typename T> T* append(packet_buffer* buffer)
{
    return (reinterpret_cast<T*>(append(buffer, sizeof(T))));
}

template <typename T> T* front(packet_buffer* buffer)
{
    return (reinterpret_cast<T*>(front(buffer)));
}

/**
 * Tx functions, e.g. setters
 */
void length(packet_buffer* buffer, uint16_t length);

union header_lengths
{
    uint64_t value = 0;
    struct
    {
        uint64_t layer2 : 7;
        uint64_t layer3 : 9;
        uint64_t layer4 : 8;
        uint64_t tso_segsz : 16;
        uint64_t unused : 24;
    };
};

void tx_offload(packet_buffer* buffer,
                header_lengths hdr_lens,
                packet_type::flags flags);

void signature(packet_buffer* buffer,
               uint32_t stream_id,
               uint32_t seq_num,
               int flags);

/**
 * Rx functions; some values are only set on receive path
 */
uint16_t length(const packet_buffer* buffer);

timesync::chrono::realtime::time_point
rx_timestamp(const packet_buffer* buffer);

uint32_t rss_hash(const packet_buffer* buffer);

std::optional<uint32_t> signature_stream_id(const packet_buffer* buffer);

std::optional<uint32_t> signature_sequence_number(const packet_buffer* buffer);

std::optional<timesync::chrono::realtime::time_point>
signature_tx_timestamp(const packet_buffer* buffer);

} // namespace openperf::packetio::packet

#endif /* _OP_PACKETIO_PACKET_BUFFER_HPP_ */
