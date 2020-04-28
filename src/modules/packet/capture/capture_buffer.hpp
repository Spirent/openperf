#ifndef _OP_PACKET_CAPTURE_BUFFER_HPP_
#define _OP_PACKET_CAPTURE_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <queue>
#include <assert.h>

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

// Pad capture data length to 4 byte boundaries
static inline uint32_t pad_capture_data_len(uint32_t length)
{
    return (length + 3) & ~(uint32_t)0x03;
}

struct capture_buffer_stats
{
    uint64_t packets;
    uint64_t bytes;
};

/**
 * The burst_holder class holds a burst of packets and
 * and keeps track of how much of the burst has been
 * processed.
 */
template <typename PacketPtrType, int BurstSize> class burst_holder
{
public:
    bool is_empty() const { return (offset == count); }

    size_t available() const { return (count - offset); }

    bool next()
    {
        if (++offset >= count) {
            offset = count = 0;
            return false;
        }
        return true;
    }

    auto get()
    {
        assert(count <= buffer.size());
        assert(offset < count);
        return buffer[offset];
    }

    auto get_buffer() { return buffer.data(); }

    size_t get_buffer_size() const { return buffer.size(); }

    size_t count = 0;
    size_t offset = 0;

    std::array<PacketPtrType, BurstSize> buffer;
};

/**
 * The capture_burst_reader class provides support for
 * reading a burst of packets from the reader object and
 * retrieving packets as necessary from the burst.
 *
 * This is used when it is not possible to process all
 * of the retrieved burst at once and part of the burst
 * may need to be processed later.
 */
template <class ReaderPtrType,
          typename PacketPtrType = capture_packet*,
          int BurstSize = 8>
class capture_burst_reader
{
public:
    capture_burst_reader()
        : m_reader{}
    {}
    capture_burst_reader(ReaderPtrType& reader)
        : m_reader(std::move(reader))
    {}
    capture_burst_reader(ReaderPtrType&& reader)
        : m_reader(std::move(reader))
    {
        read_packets();
    }

    capture_burst_reader(const capture_burst_reader&) = delete;
    capture_burst_reader(capture_burst_reader&& rhs) = default;
    ~capture_burst_reader() = default;

    bool is_done() const
    {
        if (!m_reader) return true;
        if (!m_packets.is_empty()) return false;
        return m_reader->is_done();
    }

    bool rewind()
    {
        m_reader->rewind();
        return read_packets();
    }

    size_t get_offset() const
    {
        return m_reader->get_offset() - m_packets.available();
    }

    bool read_packets()
    {
        assert(m_reader);
        // NOTE:
        // Reader read_packets() invalidates previous buffers!!!
        // Otherwise would be nice to be able to shift packets and
        // fill partially filled buffer.
        m_packets.count = m_reader->read_packets(m_packets.get_buffer(),
                                                 m_packets.get_buffer_size());
        m_packets.offset = 0;

        return (m_packets.count > 0);
    }

    PacketPtrType get() { return m_packets.get(); }

    bool next()
    {
        assert(m_reader);
        if (m_packets.next()) return true;
        if (m_reader->is_done()) return false;
        return read_packets();
    }

    auto get_stats() const { return m_reader->get_stats(); }

    ReaderPtrType m_reader;
    burst_holder<PacketPtrType, BurstSize> m_packets;
};

/**
 * The capture_reader_iterator class implements an iterator
 * interface using the provided reader object.
 *
 * If no reader object is passed to the constructor the
 * capture_reader_iterator objects represents the end
 * of the capture.
 */
template <class ReaderPtrType,
          typename PacketPtrType = capture_packet*,
          int BurstSize = 8>
class capture_reader_iterator
    : private capture_burst_reader<ReaderPtrType, PacketPtrType, BurstSize>
{
    using base_type =
        capture_burst_reader<ReaderPtrType, PacketPtrType, BurstSize>;

public:
    capture_reader_iterator() {}
    capture_reader_iterator(ReaderPtrType& reader)
        : base_type(std::move(reader))
    {}
    capture_reader_iterator(ReaderPtrType&& reader)
        : base_type(std::move(reader))
    {}

    capture_reader_iterator(const capture_reader_iterator&) = delete;
    capture_reader_iterator(capture_reader_iterator&& rhs) = default;
    ~capture_reader_iterator() = default;

    capture_reader_iterator& operator=(capture_reader_iterator&& rhs) = default;
    capture_reader_iterator&
    operator=(const capture_reader_iterator&) const = delete;

    bool operator==(const capture_reader_iterator& rhs) const
    {
        if (base_type::is_done() != rhs.is_done()) return false;
        if (base_type::is_done()) return true;
        return base_type::get_offset() == rhs.get_offset();
    }

    bool operator!=(const capture_reader_iterator& rhs) const
    {
        return !(*this == rhs);
    }

    capture_reader_iterator& operator++()
    {
        base_type::next();
        return *this;
    }

    typename std::remove_pointer<PacketPtrType>::type& operator*()
    {
        return *base_type::get();
    }

    PacketPtrType operator->() { return base_type::get(); }
};

/**
 * The capture_buffer_reader class is abstract class
 * which provides an interface from reading packets
 * from a capture_buffer.
 */
class capture_buffer_reader
{
public:
    using iterator = capture_reader_iterator<capture_buffer_reader*>;

    virtual ~capture_buffer_reader() = default;

    /**
     * Checks if the reader has completed reading the capture buffer.
     * @return true when done reading the capture buffer or
     *         false if there are more packets to read.
     */
    virtual bool is_done() const = 0;

    /**
     * Reads packets from the capture buffer.
     *
     * The reader may cache data for the returned packets.
     * Therefore the returned packets are only guaranteed to be valid
     * until read_packets() is called again or rewind() is called.
     *
     * @param[out] packets The packets which were read.
     * @param[in] count The size of the packets array.
     * @return The number of packets which were read.
     */
    virtual size_t read_packets(capture_packet* packets[], size_t count) = 0;

    /**
     * Get stats from the attached capture_buffer object.
     */
    virtual capture_buffer_stats get_stats() const = 0;

    /**
     * Get the current read offset in bytes.
     */
    virtual size_t get_offset() const = 0;

    /**
     * Rewind the capture_reader to the start of the capture buffer.
     */
    virtual void rewind() = 0;

    /**
     * Gets an iterator object for iterating over the capture.
     *
     * The iterator calls the read_packets() function,
     * so when an iterator is being used the read_packets() function
     * should not be called directly.
     */
    iterator begin() { return iterator(this); }

    /**
     * Gets an iterator object representing the end of the capture.
     */
    iterator end() { return iterator(); }
};

/**
 * The capture_buffer class is abstract class
 * which provides an interface for writing packets to
 * and reading them back.
 */
class capture_buffer
{
public:
    using iterator =
        capture_reader_iterator<std::unique_ptr<capture_buffer_reader>>;

    virtual ~capture_buffer() = default;

    /**
     * Writes packets to the capture buffer
     * @param packets The packets to write.
     * @param packets_length The number of packets to write.
     * @return The number of packets which were written.
     */
    virtual int write_packets(
        const openperf::packetio::packets::packet_buffer* const packets[],
        uint16_t packets_length) = 0;

    /**
     * Checks if the capture buffer is full.
     * @return true when the buffer is full or
     *         false if the buffer is not full.
     */
    virtual bool is_full() const = 0;

    /**
     * Creates a capture_buffer_reader object to read packets from the
     * capture buffer.
     */
    virtual std::unique_ptr<capture_buffer_reader> create_reader() = 0;

    /**
     * Gets stats for the capture buffer
     */
    virtual capture_buffer_stats get_stats() const = 0;

    /**
     * Gets an iterator object to iterate over the capture.
     */
    iterator begin() { return iterator(create_reader()); }

    /**
     * Gets an iterator object representing the end of the capture.
     */
    iterator end() { return iterator(); }
};

/**
 * Capture buffer implementation which writes to a memory.
 */
class capture_buffer_mem : public capture_buffer
{
public:
    capture_buffer_mem(size_t size);
    capture_buffer_mem(const capture_buffer_mem&) = delete;
    virtual ~capture_buffer_mem();

    int write_packets(
        const openperf::packetio::packets::packet_buffer* const packets[],
        uint16_t packets_length) override;

    bool is_full() const override { return m_full; }

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    capture_buffer_stats get_stats() const override;

    uint8_t* get_start_addr() const { return m_start_addr; }
    uint8_t* get_cur_addr() const { return m_cur_addr; }

private:
    std::unique_ptr<uint8_t[]> m_mem;
    size_t m_mem_size;

    uint8_t* m_start_addr;
    uint8_t* m_end_addr;
    uint8_t* m_cur_addr;

    capture_buffer_stats m_stats;
    bool m_full;
};

/**
 * Capture buffer reader class for reading from a capture_buffer_mem
 * object.
 */
class capture_buffer_mem_reader : public capture_buffer_reader
{
public:
    capture_buffer_mem_reader(capture_buffer_mem& buffer);
    capture_buffer_mem_reader(const capture_buffer_mem_reader&) = delete;
    virtual ~capture_buffer_mem_reader();

    bool is_done() const override;

    size_t read_packets(capture_packet* packets[], size_t count) override;

    capture_buffer_stats get_stats() const override;

    size_t get_offset() const override { return m_read_offset; }

    void rewind() override;

private:
    capture_buffer_mem& m_buffer;
    uint8_t* m_cur_addr;
    uint8_t* m_end_addr;
    ssize_t m_read_offset;
    std::vector<capture_packet> m_packets;
    bool m_eof;
};

/**
 * Capture buffer implementation which writes to a pcap
 * file using stdio for read/write.
 *
 * This class is not intended for high performance capture.
 */
class capture_buffer_file : public capture_buffer
{
public:
    capture_buffer_file(std::string_view filename, bool keep_file = false);
    capture_buffer_file(const capture_buffer_file&) = delete;
    virtual ~capture_buffer_file();

    int write_packets(
        const openperf::packetio::packets::packet_buffer* const packets[],
        uint16_t packets_length) override;

    bool is_full() const override { return m_full; }

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    capture_buffer_stats get_stats() const override;

    const std::string get_filename() const { return m_filename; }

    void flush();

private:
    std::string m_filename;
    bool m_keep_file;
    bool m_full;
    FILE* m_fp_write;
    capture_buffer_stats m_stats;
};

/**
 * Capture buffer reader class for reading from a capture_buffer_file
 * object.
 */
class capture_buffer_file_reader : public capture_buffer_reader
{
public:
    capture_buffer_file_reader(capture_buffer_file& buffer);
    capture_buffer_file_reader(const capture_buffer_file_reader&) = delete;
    virtual ~capture_buffer_file_reader();

    bool is_done() const override;

    size_t read_packets(capture_packet* packets[], size_t count) override;

    capture_buffer_stats get_stats() const override;

    size_t get_offset() const override { return m_read_offset; }

    void rewind() override;

private:
    bool read_file_header();

    capture_buffer_file& m_buffer;
    FILE* m_fp_read;
    ssize_t m_read_offset;
    std::vector<capture_packet> m_packets;
    std::vector<uint8_t> m_packet_data;
    bool m_eof;
};

template <class ReaderHolderPtrType> struct greater_than
{
    bool operator()(ReaderHolderPtrType a, ReaderHolderPtrType b)
    {
        bool a_done = a->is_done();
        bool b_done = b->is_done();

        if (a_done || b_done) { return (int)a_done > (int)b_done; }

        return a->get()->hdr.timestamp > b->get()->hdr.timestamp;
    }
};

/**
 * Capture buffer reader class which aggregates multiple
 * capture readers.  Packets are retrieved in order by
 * timestamp.
 */
class multi_capture_buffer_reader : public capture_buffer_reader
{
public:
    multi_capture_buffer_reader(
        std::vector<std::unique_ptr<capture_buffer_reader>>&& readers);
    virtual ~multi_capture_buffer_reader() = default;

    bool is_done() const override;

    size_t read_packets(capture_packet* packets[], size_t count) override;

    capture_buffer_stats get_stats() const override;

    void rewind() override;

    size_t get_offset() const override;

private:
    void populate_timestamp_priority_list();

    using burst_reader_type =
        capture_burst_reader<std::unique_ptr<capture_buffer_reader>,
                             capture_packet*>;
    std::vector<burst_reader_type> m_readers;
    std::priority_queue<burst_reader_type*,
                        std::vector<burst_reader_type*>,
                        greater_than<burst_reader_type*>>
        m_reader_priority;
    burst_reader_type* m_reader_pending;
};

} // namespace openperf::packet::capture

#endif // _OP_PACKET_CAPTURE_BUFFER_HPP_