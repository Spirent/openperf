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

class capture_buffer_reader
{
public:
    virtual ~capture_buffer_reader() = default;

    virtual bool is_done() const = 0;

    virtual capture_packet& get() = 0;

    virtual bool next() = 0;

    virtual capture_buffer_stats get_stats() const = 0;

    virtual size_t get_offset() const = 0;

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

class capture_buffer_iterator
{
public:
    capture_buffer_iterator() {}
    capture_buffer_iterator(std::unique_ptr<capture_buffer_reader>& reader)
        : m_reader(std::move(reader))
    {}
    capture_buffer_iterator(std::unique_ptr<capture_buffer_reader>&& reader)
        : m_reader(std::move(reader))
    {}

    capture_buffer_iterator(const capture_buffer_iterator&) = delete;
    capture_buffer_iterator(capture_buffer_iterator&& rhs) = default;
    ~capture_buffer_iterator() = default;

    capture_buffer_iterator& operator=(capture_buffer_iterator&& rhs) = default;
    capture_buffer_iterator&
    operator=(const capture_buffer_iterator&) const = delete;

    bool operator==(const capture_buffer_iterator& rhs) const
    {
        if (is_eof() != rhs.is_eof()) return false;
        if (is_eof()) return true;
        return m_reader->get_offset() == rhs.m_reader->get_offset();
    }

    bool operator!=(const capture_buffer_iterator& rhs) const
    {
        return !(*this == rhs);
    }

    capture_buffer_iterator& operator++()
    {
        if (!is_eof()) { m_reader->next(); }
        return *this;
    }

    capture_packet& operator*() { return m_reader->get(); }
    capture_packet* operator->() { return &m_reader->get(); }

private:
    bool is_eof() const
    {
        if (m_reader.get() == nullptr) return true;
        return m_reader->is_done();
    }

    std::unique_ptr<capture_buffer_reader> m_reader;
};

class capture_buffer
{
public:
    using iterator = capture_buffer_iterator;

    virtual ~capture_buffer() = default;

    virtual int write_packets(
        const openperf::packetio::packets::packet_buffer* const packets[],
        uint16_t packets_length) = 0;

    virtual std::unique_ptr<capture_buffer_reader> create_reader() = 0;

    virtual capture_buffer_stats get_stats() const = 0;

    iterator begin() { return iterator(create_reader()); }

    iterator end() { return iterator(); }
};

class capture_buffer_file : public capture_buffer
{
public:
    capture_buffer_file(std::string_view filename, bool keep_file = false);
    capture_buffer_file(const capture_buffer_file&) = delete;
    virtual ~capture_buffer_file();

    int write_packets(
        const openperf::packetio::packets::packet_buffer* const packets[],
        uint16_t packets_length) override;

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    capture_buffer_stats get_stats() const override;

    const std::string get_filename() const { return m_filename; }

    void flush();

private:
    std::string m_filename;
    bool m_keep_file;
    FILE* m_fp_write;
    capture_buffer_stats m_stats;
};

class capture_buffer_file_reader : public capture_buffer_reader
{
public:
    capture_buffer_file_reader(capture_buffer_file& buffer);
    capture_buffer_file_reader(const capture_buffer_file_reader&) = delete;
    virtual ~capture_buffer_file_reader();

    bool is_done() const override;

    capture_packet& get() override;

    bool next() override;

    capture_buffer_stats get_stats() const override;

    size_t get_offset() const override { return m_read_offset; }

    void rewind() override;

private:
    bool read_file_header();

    capture_buffer_file& m_buffer;
    FILE* m_fp_read;
    ssize_t m_read_offset;
    capture_packet m_capture_packet;
    std::vector<uint8_t> m_read_buffer;
    bool m_eof;
};

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

    size_t get_offset() const override;

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
