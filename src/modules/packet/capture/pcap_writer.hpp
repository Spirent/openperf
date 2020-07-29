#ifndef _OP_PCAP_WRITER_HPP_
#define _OP_PCAP_WRITER_HPP_

#include "packet/capture/pcap_defs.hpp"

namespace openperf::packet::capture {
struct capture_packet;
}

namespace openperf::packet::capture::pcap {

class pcap_buffer_writer
{
public:
    bool write_file_header();

    bool write_section_block();

    bool write_interface_block();

    bool write_packet(const capture_packet& packet);

    bool write_packet_block(const enhanced_packet_block& block_hdr,
                            const uint8_t* block_data,
                            const void* options_data,
                            size_t options_length);

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
            return pad_block_length(sizeof(section_block) + sizeof(uint32_t))
                   + pad_block_length(sizeof(interface_description_block)
                                      + sizeof(uint32_t)
                                      + sizeof(interface_default_options));
        }
        static size_t packet_length(size_t captured_len)
        {
            return (sizeof(enhanced_packet_block)
                    + pad_block_length(captured_len)
                    + sizeof(enhanced_packet_block_default_options)
                    + sizeof(uint32_t));
        }
        static size_t file_trailer_length() { return 0; }
    };

private:
    std::array<uint8_t, 16384> m_buffer;
    size_t m_buffer_length = 0;
};

} // namespace openperf::packet::capture::pcap

#endif // _OP_PCAP_WRITER_HPP_
