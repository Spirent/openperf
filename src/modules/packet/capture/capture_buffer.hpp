#ifndef _OP_PACKET_CAPTURE_BUFFER_HPP_
#define _OP_PACKET_CAPTURE_BUFFER_HPP_

#include <cstdint>
#include <string>
#include <optional>
#include <vector>
#include <queue>
#include <assert.h>

#include "timesync/chrono.hpp"

namespace openperf::packetio::packet {
struct packet_buffer;
}

namespace openperf::packet::capture {

using clock = openperf::timesync::chrono::realtime;

const uint64_t CAPTURE_FLAG_TX = 0x01;

struct capture_packet_hdr
{
    clock::time_point timestamp;
    uint32_t captured_len;
    uint32_t packet_len;
    uint64_t flags;
};

struct capture_packet
{
    capture_packet_hdr hdr;
    const uint8_t* data;
};

// Pad capture data length to 4 byte boundaries
constexpr uint32_t pad_capture_data_len(uint32_t length)
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
template <typename PacketPtrType, uint16_t BurstSize> class burst_holder
{
public:
    bool is_empty() const { return (offset == count); }

    uint16_t available() const { return (count - offset); }

    bool next()
    {
        if (++offset >= count) {
            offset = count = 0;
            return false;
        }
        return true;
    }

    PacketPtrType get() const
    {
        assert(count <= buffer.size());
        assert(offset < count);
        return buffer[offset];
    }

    auto get_buffer() { return buffer.data(); }

    size_t get_buffer_size() const { return buffer.size(); }

    uint16_t count = 0;
    uint16_t offset = 0;

    // std::remove_const<> for pointers doesn't remove the const before the type
    using PacketType = typename std::remove_const<
        typename std::remove_pointer<PacketPtrType>::type>::type;

    std::array<PacketType*, BurstSize> buffer;
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
          uint16_t BurstSize = 8>
class capture_burst_reader
{
public:
    capture_burst_reader()
        : m_reader{}
    {}
    capture_burst_reader(ReaderPtrType&& reader)
        : m_reader(std::forward<ReaderPtrType>(reader))
    {
        read_packets();
    }

    capture_burst_reader(const capture_burst_reader&) = delete;
    capture_burst_reader(capture_burst_reader&& rhs) = default;
    ~capture_burst_reader() = default;

    bool is_done() const
    {
        if (!m_reader) return true;
        return m_packets.is_empty();
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

    const PacketPtrType get() const { return m_packets.get(); }

    bool next()
    {
        if (m_packets.next()) return true;
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
template <class ReaderPtrType, typename PacketPtrType, uint16_t BurstSize = 8>
class capture_reader_iterator
    : private capture_burst_reader<ReaderPtrType, PacketPtrType, BurstSize>
{
    using base_type =
        capture_burst_reader<ReaderPtrType, PacketPtrType, BurstSize>;

public:
    // iterator traits
    using difference_type = std::ptrdiff_t;
    using value_type = typename std::remove_pointer<PacketPtrType>::type;
    using pointer = PacketPtrType;
    using reference = value_type&;
    using iterator_category = std::forward_iterator_tag;

    capture_reader_iterator() {}
    capture_reader_iterator(ReaderPtrType&& reader)
        : base_type(std::forward<ReaderPtrType>(reader))
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

    reference operator*() const { return *base_type::get(); }

    pointer operator->() const { return base_type::get(); }
};

/**
 * The capture_buffer_reader class is abstract class
 * which provides an interface from reading packets
 * from a capture_buffer.
 */
class capture_buffer_reader
{
public:
    using const_iterator =
        capture_reader_iterator<capture_buffer_reader*, const capture_packet*>;

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
    virtual uint16_t read_packets(capture_packet* packets[],
                                  uint16_t count) = 0;

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
    const_iterator begin() { return const_iterator(this); }

    /**
     * Gets an iterator object representing the end of the capture.
     */
    const_iterator end() { return const_iterator(); }
};

/**
 * The capture_buffer class is abstract class
 * which provides an interface for writing packets to
 * and reading them back.
 */
class capture_buffer
{
public:
    using const_iterator =
        capture_reader_iterator<std::unique_ptr<capture_buffer_reader>,
                                const capture_packet*>;

    virtual ~capture_buffer() = default;

    /**
     * Writes packets to the capture buffer
     * @param packets The packets to write.
     * @param packets_length The number of packets to write.
     * @return The number of packets which were written.
     */
    virtual uint16_t write_packets(
        const openperf::packetio::packet::packet_buffer* const packets[],
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
    const_iterator begin() { return const_iterator(create_reader()); }

    /**
     * Gets an iterator object representing the end of the capture.
     */
    const_iterator end() { return const_iterator(); }
};

/**
 * Capture buffer implementation which writes to a memory.
 */
class capture_buffer_mem : public capture_buffer
{
public:
    capture_buffer_mem(uint64_t size, uint32_t max_packet_size = UINT32_MAX);
    capture_buffer_mem(const capture_buffer_mem&) = delete;
    virtual ~capture_buffer_mem();

    uint16_t write_packets(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) override;

    bool is_full() const override { return m_full; }

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    capture_buffer_stats get_stats() const override;

    uint8_t* get_start_addr() const { return m_start_addr; }
    uint8_t* get_end_addr() const { return m_end_addr; }
    uint8_t* get_write_addr() const { return m_write_addr; }

protected:
    uint8_t* m_mem;
    uint64_t m_mem_size;

    uint8_t* m_start_addr;
    uint8_t* m_end_addr;
    uint8_t* m_write_addr;

    capture_buffer_stats m_stats;
    uint32_t m_max_packet_size;
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
    virtual ~capture_buffer_mem_reader() = default;

    bool is_done() const override;

    uint16_t read_packets(capture_packet* packets[], uint16_t count) override;

    capture_buffer_stats get_stats() const override;

    size_t get_offset() const override { return m_read_offset; }

    void rewind() override;

protected:
    void init();

    capture_buffer_mem& m_buffer;
    uint8_t* m_read_addr;
    uint8_t* m_end_addr;
    ssize_t m_read_offset;
    std::vector<capture_packet> m_packets;
    bool m_eof;
};

/**
 * Capture buffer implementation which writes to a memory and wraps
 */
class capture_buffer_mem_wrap : public capture_buffer_mem
{
public:
    capture_buffer_mem_wrap(uint64_t size,
                            uint32_t max_packet_size = UINT32_MAX);
    capture_buffer_mem_wrap(const capture_buffer_mem_wrap&) = delete;
    virtual ~capture_buffer_mem_wrap() = default;

    uint16_t write_packets(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) override;

    bool is_full() const override { return m_full; }

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    uint8_t* get_wrap_addr() const { return m_wrap_addr; }
    uint8_t* get_wrap_end_addr() const { return m_wrap_end_addr; }

private:
    std::pair<size_t, size_t> count_packets_and_bytes(uint8_t* start,
                                                      uint8_t* end);
    void make_space_if_needed(size_t required_size);

    uint8_t* m_wrap_addr;
    uint8_t* m_wrap_end_addr;
};

/**
 * Capture buffer reader class for reading from a capture_buffer_mem
 * object.
 */
class capture_buffer_mem_wrap_reader : public capture_buffer_reader
{
public:
    capture_buffer_mem_wrap_reader(capture_buffer_mem_wrap& buffer);
    capture_buffer_mem_wrap_reader(const capture_buffer_mem_wrap_reader&) =
        delete;
    virtual ~capture_buffer_mem_wrap_reader() = default;

    bool is_done() const override;

    uint16_t read_packets(capture_packet* packets[], uint16_t count) override;

    capture_buffer_stats get_stats() const override;

    size_t get_offset() const override { return m_read_offset; }

    void rewind() override;

protected:
    void init();

    capture_buffer_mem_wrap& m_buffer;
    uint8_t* m_start_addr;
    uint8_t* m_end_addr;
    uint8_t* m_wrap_addr;
    uint8_t* m_read_addr;
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
    enum class keep_file { disabled, enabled };

    capture_buffer_file(std::string_view filename,
                        keep_file keep_file = keep_file::disabled,
                        uint32_t max_packet_size = UINT32_MAX);
    capture_buffer_file(const capture_buffer_file&) = delete;
    virtual ~capture_buffer_file();

    uint16_t write_packets(
        const openperf::packetio::packet::packet_buffer* const packets[],
        uint16_t packets_length) override;

    bool is_full() const override { return m_full; }

    std::unique_ptr<capture_buffer_reader> create_reader() override;

    capture_buffer_stats get_stats() const override;

    const std::string get_filename() const { return m_filename; }

    void flush();

private:
    std::string m_filename;
    keep_file m_keep_file;
    bool m_full;
    uint32_t m_max_packet_size;
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

    uint16_t read_packets(capture_packet* packets[], uint16_t count) override;

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

        if (a_done || b_done) { return a_done > b_done; }

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

    uint16_t read_packets(capture_packet* packets[], uint16_t count) override;

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
