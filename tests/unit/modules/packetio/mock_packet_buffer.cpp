#include "packetio/mock_packet_buffer.hpp"

namespace openperf::packetio::packet {

uint16_t max_length(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)->data_length;
}

uint16_t length(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)->length;
}

void length(packet_buffer* buffer, uint16_t length)
{
    reinterpret_cast<mock_packet_buffer*>(buffer)->length = length;
}

timesync::chrono::realtime::time_point rx_timestamp(const packet_buffer* buffer)
{
    using clock = openperf::timesync::chrono::realtime;
    return (clock::time_point{clock::duration{
        reinterpret_cast<const mock_packet_buffer*>(buffer)->rx_timestamp}});
}

uint32_t rss_hash(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)->rss_hash;
}

uint32_t type(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)->type;
}

std::optional<uint32_t> signature_stream_id(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)
        ->signature_stream_id;
}

std::optional<uint32_t> signature_sequence_number(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)
        ->signature_sequence_number;
}

std::optional<timesync::chrono::realtime::time_point>
signature_tx_timestamp(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)
        ->signature_tx_timestamp;
}

void* to_data(packet_buffer* buffer)
{
    return reinterpret_cast<mock_packet_buffer*>(buffer)->data;
}

const void* to_data(const packet_buffer* buffer)
{
    return reinterpret_cast<const mock_packet_buffer*>(buffer)->data;
}

void* to_data(packet_buffer* buffer, uint16_t offset)
{
    if (offset >= length(buffer)) { return nullptr; }
    auto data = reinterpret_cast<uint8_t*>(to_data(buffer)) + offset;
    return data;
}

const void* to_data(const packet_buffer* buffer, uint16_t offset)
{
    if (offset >= length(buffer)) { return nullptr; }
    auto data = reinterpret_cast<const uint8_t*>(to_data(buffer)) + offset;
    return data;
}

void* prepend(packet_buffer* buffer, uint16_t offset)
{
    // Currently not supported
    return 0;
}

void* append(packet_buffer* buffer, uint16_t offset)
{
    // Currently not supported
    return 0;
}

void* front(packet_buffer* buffer)
{
    // Currently not supported
    return 0;
}

} // namespace openperf::packetio::packet
