#ifndef _OP_PACKET_CAPTURE_BUFFER_HPP_
#define _OP_PACKET_CAPTURE_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <queue>

namespace openperf::packetio::packets {
struct packet_buffer;
}

namespace openperf::packet::capture {

struct capture_packet_hdr
{
    uint64_t timestamp;
    uint32_t captured_len;
    uint32_t packet_len;
};

struct capture_packet
{
    capture_packet_hdr hdr;
    uint8_t* data;
};

struct capture_buffer_stats
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

        capture_packet& operator*() { return m_capture_packet; }
        capture_packet* operator->() { return &m_capture_packet; }

    private:
        iterator(capture_buffer_file& buffer, bool eof);
        bool read_file_header();
        bool next();

        FILE* m_fp_read;
        ssize_t m_read_offset;
        capture_packet m_capture_packet;
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

    capture_buffer_stats get_stats() const;

private:
    std::string m_filename;
    bool m_keep_file;
    FILE* m_fp_write;
    capture_buffer_stats m_stats;
};

class capture_buffer_reader
{
public:
    virtual ~capture_buffer_reader() = default;

    virtual bool is_done() const = 0;

    virtual capture_packet& get() = 0;

    virtual bool next() = 0;

    virtual capture_buffer_stats get_stats() const = 0;

    virtual void rewind() = 0;

    template <class T> size_t read(T&& func)
    {
        size_t count = 0;

        if (is_done()) return count;
        do {
            if (!func(get())) break;
            ++count;
        } while (next());
        return count;
    }
};

template <class T>
class capture_buffer_reader_template : public capture_buffer_reader
{
public:
    capture_buffer_reader_template(T& buffer)
        : m_buffer(buffer)
        , m_iter(buffer.begin())
        , m_end(buffer.end())
    {}

    virtual ~capture_buffer_reader_template() = default;

    bool is_done() const override { return m_iter == m_end; }

    capture_packet& get() override { return (*m_iter); }

    bool next() override
    {
        if (is_done()) return false;
        return (++m_iter) != m_end;
    }

    capture_buffer_stats get_stats() const override
    {
        return m_buffer.get_stats();
    }

    void rewind() override
    {
        m_iter = m_buffer.begin();
        m_end = m_buffer.end();
    }

private:
    T& m_buffer;
    typename T::iterator m_iter;
    typename T::iterator m_end;
};

using capture_buffer_file_reader =
    capture_buffer_reader_template<capture_buffer_file>;

struct greater_than
{
    bool operator()(capture_buffer_reader* a, capture_buffer_reader* b)
    {
        bool a_done = a->is_done();
        bool b_done = b->is_done();

        if (a_done || b_done) { return (int)a_done > (int)b_done; }

        return a->get().hdr.timestamp > b->get().hdr.timestamp;
    }
};

class multi_capture_buffer_reader : public capture_buffer_reader
{
    using reader_ptr = std::unique_ptr<capture_buffer_reader>;

public:
    multi_capture_buffer_reader(std::vector<reader_ptr>&& readers);
    virtual ~multi_capture_buffer_reader() = default;

    bool is_done() const override;

    capture_packet& get() override;

    bool next() override;

    capture_buffer_stats get_stats() const override;

    void rewind() override;

private:
    void populate_timestamp_priority_list();

    std::vector<reader_ptr> m_readers;
    std::priority_queue<capture_buffer_reader*,
                        std::vector<capture_buffer_reader*>,
                        greater_than>
        m_reader_priority;
};

} // namespace openperf::packet::capture

#endif // _OP_PACKET_CAPTURE_BUFFER_HPP_
