#ifndef _OP_PACKETIO_PACKET_BUFFER_HPP_
#define _OP_PACKETIO_PACKET_BUFFER_HPP_

#include <cstdint>
#include <cstddef>

#include "timesync/chrono.hpp"

namespace openperf::packetio::packets {

struct packet_buffer;

uint16_t max_length(const packet_buffer* buffer);

uint16_t length(const packet_buffer* buffer);
void length(packet_buffer*, uint16_t length);

timesync::chrono::realtime::time_point timestamp(const packet_buffer* buffer);

uint32_t rss_hash(const packet_buffer* buffer);

void* to_data(packet_buffer* buffer);
const void* to_data(const packet_buffer* buffer);

void* to_data(packet_buffer* buffer, uint16_t offset);
const void* to_data(const packet_buffer* buffer, uint16_t offset);

void* prepend(packet_buffer* buffer, uint16_t offset);

void* append(packet_buffer* buffer, uint16_t offset);

void* front(packet_buffer* buffer);

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

} // namespace openperf::packetio::packets

#endif /* _OP_PACKETIO_PACKET_BUFFER_HPP_ */
