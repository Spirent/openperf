#ifndef _OP_PACKETIO_MOCK_PACKET_BUFFER_HPP_
#define _OP_PACKETIO_MOCK_PACKET_BUFFER_HPP_

#include "packetio/packet_buffer.hpp"

namespace openperf::packetio::packet {

struct mock_packet_buffer
{
    uint16_t length = 0;
    uint32_t rss_hash = 0;
    uint32_t type = 0;
    uint64_t rx_timestamp = 0;
    std::optional<uint32_t> signature_stream_id;
    std::optional<uint32_t> signature_sequence_number;
    std::optional<timesync::chrono::realtime::time_point>
        signature_tx_timestamp;
    std::optional<uint32_t> prbs_octets;
    std::optional<uint32_t> prbs_bit_errors;
    uint8_t ip_chksum_bad : 1;
    uint8_t l4_chksum_bad : 1;
    uint8_t tx_sink : 1;

    uint16_t data_length = 0;
    void* data = nullptr;
};

} // namespace openperf::packetio::packet

#endif // _OP_PACKETIO_MOCK_PACKET_BUFFER_HPP_
