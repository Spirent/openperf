#ifndef _OP_PACKET_CAPTURE_BUFFER_HPP_
#define _OP_PACKET_CAPTURE_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <optional>
#include <vector>

namespace openperf::packetio::packets {
struct packet_buffer;
}

namespace openperf::packet::capture {

struct buffer_data
{
    uint64_t timestamp;
    uint32_t captured_len;
    uint32_t packet_len;
    uint8_t* data;
};

struct buffer_stats
{
    uint64_t packets;
    uint64_t bytes;
};

class capture_buffer_file
{
public:
    class iterator
    {
        friend capture_buffer_file;

    public:
        iterator(const iterator&) = delete;
        iterator(iterator&& rhs);
        ~iterator();

        iterator& operator=(iterator&& rhs);
        iterator& operator=(const iterator&) const = delete;

        bool operator==(const iterator& rhs) const
        {
            if (m_eof != rhs.m_eof) return false;
            if (m_eof) return true;
            return m_read_offset == rhs.m_read_offset;
        }
        bool operator!=(const iterator& rhs) const { return !(*this == rhs); }

        iterator& operator++()
        {
            next();
            return *this;
        }

        buffer_data& operator*() { return m_buffer_data; }
        buffer_data* operator->() { return &m_buffer_data; }

    private:
        iterator(capture_buffer_file& buffer, bool eof);
        bool read_file_header();
        bool next();

        FILE* m_fp_read;
        ssize_t m_read_offset;
        buffer_data m_buffer_data;
        std::vector<uint8_t> m_read_buffer;
        bool m_eof;
    };

public:
    capture_buffer_file(std::string_view filename, bool keep_file = false);
    ~capture_buffer_file();

    capture_buffer_file(capture_buffer_file&& rhs);
    capture_buffer_file& operator=(capture_buffer_file&& rhs);

    capture_buffer_file(const capture_buffer_file& rhs) = delete;
    capture_buffer_file& operator=(const capture_buffer_file& rhs) = delete;

    int push(const openperf::packetio::packets::packet_buffer* const packets[],
             uint16_t packets_length);

    iterator begin();
    iterator end();

    buffer_stats get_stats();

private:
    std::string m_filename;
    bool m_keep_file;
    FILE* m_fp_write;
    buffer_stats m_stats;
};

} // namespace openperf::packet::capture

#endif // _OP_PACKET_CAPTURE_BUFFER_HPP_
