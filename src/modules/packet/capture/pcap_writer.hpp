#ifndef _OP_PCAP_WRITER_HPP_
#define _OP_PCAP_WRITER_HPP_

namespace openperf::packet::capture {

namespace pcapng {
struct enhanced_packet_block;
}

struct capture_packet;
class capture_buffer_reader;

class pcap_buffered_writer
{
public:
    pcap_buffered_writer();

    size_t calc_length(capture_buffer_reader& reader);

    bool write_start();

    bool write_section_block();

    bool write_interface_block();

    bool write_packet(const capture_packet& packet);

    bool write_packet_block(const pcapng::enhanced_packet_block& block_hdr,
                            const uint8_t* block_data);

    bool write_end() { return true; }

    const uint8_t* get_data() const { return m_buffer.data(); };
    size_t get_length() const { return m_buffer_length; }
    size_t get_available_length() const
    {
        return m_buffer.size() - m_buffer_length;
    }
    void flush() { m_buffer_length = 0; }

private:
    std::array<uint8_t, 16384> m_buffer;
    size_t m_buffer_length = 0;
};

} // namespace openperf::packet::capture

#endif // _OP_PCAP_WRITER_HPP_