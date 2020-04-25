#ifndef _OP_PCAP_WRITER_HPP_
#define _OP_PCAP_WRITER_HPP_

#include "packet/capture/pcap_io.hpp"

namespace openperf::packet::capture {

struct capture_packet;
class capture_buffer_reader;

class pcap_buffer_writer
{
public:
    pcap_buffer_writer();

    bool write_file_header();

    bool write_section_block();

    bool write_interface_block();

    bool write_packet(const capture_packet& packet);

    bool write_packet_block(const pcapng::enhanced_packet_block& block_hdr,
                            const uint8_t* block_data);

    bool write_file_trailer() { return true; }

    const uint8_t* get_data() const { return m_buffer.data(); };
    size_t get_length() const { return m_buffer_length; }
    size_t get_available_length() const
    {
        return m_buffer.size() - m_buffer_length;
    }
    void flush() { m_buffer_length = 0; }

    struct traits
    {
        static size_t file_header_length()
        {
            return pcapng::pad_block_length(sizeof(pcapng::section_block)
                                            + sizeof(uint32_t))
                   + pcapng::pad_block_length(
                         sizeof(pcapng::interface_description_block)
                         + sizeof(uint32_t)
                         + sizeof(pcapng::interface_default_options));
        }
        static size_t packet_length(uint8_t captured_len)
        {
            return pcapng::pad_block_length(
                sizeof(pcapng::enhanced_packet_block) + sizeof(uint32_t)
                + captured_len);
        }
        static size_t file_trailer_length() { return 0; }
    };

private:
    std::array<uint8_t, 16384> m_buffer;
    size_t m_buffer_length = 0;
};

} // namespace openperf::packet::capture

#endif // _OP_PCAP_WRITER_HPP_