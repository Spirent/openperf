#include <cinttypes>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>

#include "core/op_log.h"
#include "packet/capture/capture_buffer.hpp"
#include "packet/capture/pcap_defs.hpp"
#include "packetio/packet_buffer.hpp"
#include "utils/filesystem.hpp"

// When OP_CAPTURE_USE_MLOCK is defined,  mlock() will be used on the capture
// buffer memory to avoid paging.  If OP_CAPTURE_USE_MLOCK is not defined,
// madvise() will be used to tell the kernel that the memory will be accessed
// sequentially.
//#define OP_CAPTURE_USE_MLOCK

namespace openperf::packet::capture {

constexpr size_t MAX_PACKET_SIZE = 16384;

/**
 * Round x up to the nearest multiple of 'multiple'.
 */
template <typename T> static __attribute__((const)) T round_up(T x, T multiple)
{
    assert(multiple); /* don't want a multiple of 0 */
    auto remainder = x % multiple;
    return (remainder == 0 ? x : x + multiple - remainder);
}

///////////////////////////////////////////////////////////////////////////////

capture_buffer_mem::capture_buffer_mem(uint64_t size, uint32_t max_packet_size)
    : m_mem(nullptr)
    , m_mem_size(0)
    , m_start_addr(nullptr)
    , m_end_addr(nullptr)
    , m_write_addr(nullptr)
    , m_stats{0, 0}
    , m_max_packet_size(max_packet_size)
    , m_full(false)
{
    const size_t page_size = sysconf(_SC_PAGESIZE);
    m_mem_size = round_up(size, page_size);
    auto addr = mmap(nullptr,
                     m_mem_size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
                     -1,
                     0);
    if (addr == MAP_FAILED) {
        OP_LOG(OP_LOG_ERROR,
               "Failed to mmap capture buffer memory size %zu.  %s",
               m_mem_size,
               strerror(errno));
        throw std::bad_alloc();
    }
    m_mem = reinterpret_cast<uint8_t*>(addr);
    OP_LOG(OP_LOG_INFO,
           "allocated capture buffer memory address %p size %zu",
           reinterpret_cast<void*>(m_mem),
           m_mem_size);

#ifdef OP_CAPTURE_USE_MLOCK
    if (mlock(m_mem, m_mem_size) < 0) {
        OP_LOG(OP_LOG_WARNING,
               "Unable to mlock capture memory address %p size %zu",
               reinterpret_cast<void*>(m_mem),
               m_mem_size);
    }
#else
    if (madvise(m_mem, m_mem_size, MADV_SEQUENTIAL | MADV_WILLNEED) != 0) {
        OP_LOG(OP_LOG_WARNING,
               "Unable to madvise capture memory address %p size %zu",
               reinterpret_cast<void*>(m_mem),
               m_mem_size);
    }
#endif

    m_start_addr = m_mem;
    m_end_addr = m_mem + m_mem_size;
    m_write_addr = m_start_addr;
}

capture_buffer_mem::~capture_buffer_mem()
{
    if (m_mem) {
#ifdef OP_CAPTURE_USE_MLOCK
        munlock(m_mem, m_mem_size);
#endif
        if (munmap(m_mem, m_mem_size) != 0) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed to unmap capture buffer memory %p size %zu.  %s",
                   reinterpret_cast<void*>(m_mem),
                   m_mem_size,
                   strerror(errno));
        }
        m_mem = nullptr;
    }
}

static inline void
fill_capture_packet_hdr(capture_packet_hdr& hdr,
                        const openperf::packetio::packet::packet_buffer* packet,
                        const uint32_t max_packet_size)
{
    hdr.timestamp = openperf::packetio::packet::rx_timestamp(packet);
    hdr.packet_len = openperf::packetio::packet::length(packet);
    hdr.captured_len = std::min(hdr.packet_len, max_packet_size);
    hdr.flags = 0;
    // RX = 1, TX = 2
    hdr.dir = 1 << openperf::packetio::packet::tx_sink(packet);
}

uint16_t capture_buffer_mem::write_packets(
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length)
{
    capture_packet_hdr hdr;

    if (m_full) { return 0; }

    // Write all packets or until the buffer is full
    for (uint16_t i = 0; i < packets_length; ++i) {
        auto& packet = packets[i];
        auto data = openperf::packetio::packet::to_data(packet);

        fill_capture_packet_hdr(hdr, packet, m_max_packet_size);
        auto padded_data_len = pad_capture_data_len(hdr.captured_len);
        if (m_write_addr + sizeof(hdr) + padded_data_len > m_end_addr) {
            // The buffer is full, so don't add any more packets
            m_full = true;
            return i;
        }
        *reinterpret_cast<capture_packet_hdr*>(m_write_addr) = hdr;
        m_write_addr += sizeof(hdr);
        std::copy_n(reinterpret_cast<const uint8_t*>(data),
                    hdr.captured_len,
                    m_write_addr);
        m_write_addr += padded_data_len;
        m_stats.packets += 1;
        m_stats.bytes += hdr.captured_len;
    }

    return packets_length;
}

std::unique_ptr<capture_buffer_reader> capture_buffer_mem::create_reader()
{
    return std::make_unique<capture_buffer_mem_reader>(*this);
}

capture_buffer_stats capture_buffer_mem::get_stats() const { return m_stats; }

///////////////////////////////////////////////////////////////////////////////

capture_buffer_mem_reader::capture_buffer_mem_reader(capture_buffer_mem& buffer)
    : m_buffer(buffer)
    , m_read_addr(nullptr)
    , m_end_addr(nullptr)
    , m_read_offset(0)
    , m_eof(false)
{
    init();
}

bool capture_buffer_mem_reader::is_done() const { return m_eof; }

uint16_t capture_buffer_mem_reader::read_packets(capture_packet* packets[],
                                                 uint16_t count)
{
    if (count > m_packets.size()) {
        // Allocate buffer space for all packets
        m_packets.resize(count);
    }

    uint16_t i = 0;
    while (i < count) {
        auto& packet = m_packets[i];
        if (m_read_addr + sizeof(packet.hdr) > m_end_addr) {
            m_eof = true;
            break;
        }
        packet.hdr = *reinterpret_cast<capture_packet_hdr*>(m_read_addr);
        m_read_addr += sizeof(packet.hdr);
        if (m_read_addr + packet.hdr.captured_len > m_end_addr) {
            m_eof = true;
            break;
        }
        packet.data = m_read_addr;
        m_read_addr += pad_capture_data_len(packet.hdr.captured_len);
        packets[i] = &packet;
        ++i;
        m_read_offset += pad_capture_data_len(packet.hdr.captured_len);
    }
    return i;
}

capture_buffer_stats capture_buffer_mem_reader::get_stats() const
{
    return m_buffer.get_stats();
}

void capture_buffer_mem_reader::init()
{
    m_read_offset = 0;
    m_read_addr = m_buffer.get_start_addr();
    m_end_addr = m_buffer.get_write_addr();
    m_eof = false;
}

void capture_buffer_mem_reader::rewind() { init(); }

///////////////////////////////////////////////////////////////////////////////

capture_buffer_mem_wrap::capture_buffer_mem_wrap(uint64_t size,
                                                 uint32_t max_packet_size)
    : capture_buffer_mem(size, max_packet_size)
    , m_wrap_addr(nullptr)
    , m_wrap_end_addr(nullptr)
{}

uint16_t capture_buffer_mem_wrap::write_packets(
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length)
{
    capture_packet_hdr hdr;

    for (uint16_t i = 0; i < packets_length; ++i) {
        auto& packet = packets[i];
        auto data = openperf::packetio::packet::to_data(packet);

        fill_capture_packet_hdr(hdr, packet, m_max_packet_size);
        auto padded_data_len = pad_capture_data_len(hdr.captured_len);
        auto total_packet_len = sizeof(hdr) + padded_data_len;
        // To simplify logic, packet hdr and data are always contiguous
        if (m_write_addr + total_packet_len > m_end_addr) {
            if (m_wrap_addr > m_start_addr) {
                // Remove old packets till end of buffer because these are
                // considered discarded when wrap occurs
                auto [packets, bytes] =
                    count_packets_and_bytes(m_wrap_addr, m_wrap_end_addr);
                m_stats.packets -= packets;
                m_stats.bytes -= bytes;
            }
            m_wrap_addr = m_start_addr;
            m_wrap_end_addr = m_write_addr;
            m_write_addr = m_start_addr;
        }
        make_space_if_needed(total_packet_len);

        *reinterpret_cast<capture_packet_hdr*>(m_write_addr) = hdr;
        m_write_addr += sizeof(hdr);
        std::copy_n(reinterpret_cast<const uint8_t*>(data),
                    hdr.captured_len,
                    m_write_addr);
        m_write_addr += padded_data_len;
        m_stats.packets += 1;
        m_stats.bytes += hdr.captured_len;
    }

    return packets_length;
}

std::pair<size_t, size_t>
capture_buffer_mem_wrap::count_packets_and_bytes(uint8_t* start, uint8_t* end)
{
    std::pair<size_t, size_t> result{0, 0};
    auto& [packets, bytes] = result;
    uint8_t* p = start;
    while (p < end) {
        auto* hdr = reinterpret_cast<capture_packet_hdr*>(p);
        auto total_len = sizeof(*hdr) + pad_capture_data_len(hdr->captured_len);
        assert(p + total_len <= end);
        ++packets;
        bytes += hdr->captured_len;
        p += total_len;
    }

    return result;
}

void capture_buffer_mem_wrap::make_space_if_needed(size_t required_size)
{
    if (m_write_addr > m_wrap_addr) {
        // If current address is > wrap address then only need to worry about
        // buff end caller should take care of wrap
        assert(m_write_addr + required_size <= m_end_addr);
        return;
    }
    size_t available_size = m_wrap_addr - m_write_addr;
    size_t packets = 0;
    size_t bytes = 0;
    auto p = m_wrap_addr;
    assert(m_wrap_addr);

    // Remove packets until there is space or end of buffer is reached
    while (available_size < required_size) {
        if (p + sizeof(capture_packet_hdr) > m_wrap_end_addr) {
            // Reached end of buffer
            assert(p < m_end_addr);
            available_size += (m_end_addr - p);
            assert(available_size >= required_size);
            p = m_start_addr;
            break;
        }
        auto* hdr = reinterpret_cast<capture_packet_hdr*>(p);
        auto total_len = sizeof(*hdr) + pad_capture_data_len(hdr->captured_len);
        assert(p + total_len <= m_wrap_end_addr);
        available_size += total_len;
        ++packets;
        bytes += hdr->captured_len;
        p += total_len;
    }
    m_wrap_addr = p;
    m_stats.packets -= packets;
    m_stats.bytes -= bytes;
}

std::unique_ptr<capture_buffer_reader> capture_buffer_mem_wrap::create_reader()
{
    return std::make_unique<capture_buffer_mem_wrap_reader>(*this);
}

///////////////////////////////////////////////////////////////////////////////

capture_buffer_mem_wrap_reader::capture_buffer_mem_wrap_reader(
    capture_buffer_mem_wrap& buffer)
    : m_buffer(buffer)
    , m_start_addr(nullptr)
    , m_end_addr(nullptr)
    , m_wrap_addr(nullptr)
    , m_read_addr(nullptr)
    , m_read_offset(0)
    , m_eof(false)
{
    init();
}

bool capture_buffer_mem_wrap_reader::is_done() const { return m_eof; }

uint16_t capture_buffer_mem_wrap_reader::read_packets(capture_packet* packets[],
                                                      uint16_t count)
{
    if (count > m_packets.size()) {
        // Allocate buffer space for all packets
        m_packets.resize(count);
    }

    uint16_t i = 0;
    while (i < count) {
        auto& packet = m_packets[i];
        auto end_addr = m_end_addr;
        if (m_wrap_addr) {
            if (m_read_addr > m_wrap_addr) {
                // After the wrap location
                if (m_read_addr + sizeof(packet.hdr) > m_end_addr) {
                    m_read_addr = m_start_addr;
                    continue;
                }
            } else if (m_read_addr < m_wrap_addr) {
                // Before the wrap location
                end_addr = m_wrap_addr;
                if (m_read_addr + sizeof(packet.hdr) > m_wrap_addr) {
                    m_eof = true;
                    break;
                }
            } else {
                // On the wrap location
                // It is possible the start and end could be the same place
                // So either just starting or reached the end.
                if (m_read_offset != 0) {
                    // If any packets were read then this is the end
                    m_eof = true;
                    break;
                }
                if (m_read_addr + sizeof(packet.hdr) > m_end_addr) {
                    m_read_addr = m_start_addr;
                    continue;
                }
            }
        } else {
            // Simple case with no buffer wrapping
            if (m_read_addr + sizeof(packet.hdr) > m_end_addr) {
                m_eof = true;
                break;
            }
        }
        packet.hdr = *reinterpret_cast<capture_packet_hdr*>(m_read_addr);
        m_read_addr += sizeof(packet.hdr);
        if (m_read_addr + packet.hdr.captured_len > end_addr) {
            m_eof = true;
            break;
        }
        packet.data = m_read_addr;
        m_read_addr += pad_capture_data_len(packet.hdr.captured_len);
        packets[i] = &packet;
        ++i;
        m_read_offset += pad_capture_data_len(packet.hdr.captured_len);
    }
    return i;
}

capture_buffer_stats capture_buffer_mem_wrap_reader::get_stats() const
{
    return m_buffer.get_stats();
}

void capture_buffer_mem_wrap_reader::init()
{
    m_read_offset = 0;
    bool wrapped = m_buffer.get_wrap_addr() != nullptr;

    m_start_addr = m_buffer.get_start_addr();
    if (wrapped) {
        m_end_addr =
            m_buffer
                .get_wrap_end_addr(); // This marks end of last packet in buffer
        m_wrap_addr =
            m_buffer.get_write_addr(); // Write pointer is at end of capture
        m_read_addr = m_buffer.get_wrap_addr(); // The wrap addr in buffer is
                                                // 1st packet to read
    } else {
        // No wrap detected
        m_end_addr = m_buffer.get_write_addr();
        m_read_addr = m_start_addr;
        m_wrap_addr = nullptr;
    }
    m_eof = false;
}

void capture_buffer_mem_wrap_reader::rewind() { init(); }

///////////////////////////////////////////////////////////////////////////////

capture_buffer_file::capture_buffer_file(std::string_view filename,
                                         keep_file keep_file,
                                         uint32_t max_packet_size)
    : m_filename(filename)
    , m_keep_file(keep_file)
    , m_full(false)
    , m_max_packet_size(max_packet_size)
    , m_fp_write(nullptr)
    , m_stats{0, 0}
{
    m_fp_write = fopen(m_filename.c_str(), "w+");
    if (!m_fp_write) {
        throw std::runtime_error(
            std::string("Unable to open capture buffer file ") + m_filename);
    }

    pcap::section_block section;

    auto unpadded_block_length = sizeof(section) + sizeof(uint32_t);
    auto block_length = pcap::pad_block_length(unpadded_block_length);
    auto pad_length = block_length - unpadded_block_length;

    section.block_type = pcap::block_type::SECTION;
    section.block_total_length = block_length;
    section.byte_order_magic = pcap::BYTE_ORDER_MAGIC;
    section.major_version = 1;
    section.minor_version = 0;
    section.section_length = pcap::SECTION_LENGTH_UNSPECIFIED;
    if (fwrite(&section, sizeof(section), 1, m_fp_write) != 1) {
        fclose(m_fp_write);
        m_fp_write = nullptr;
        throw std::runtime_error("Failed writing PCAP section block");
    }
    if (pad_length) {
        uint64_t pad_data = 0;
        if (fwrite(&pad_data, pad_length, 1, m_fp_write) != 1) {
            fclose(m_fp_write);
            m_fp_write = nullptr;
            throw std::runtime_error("Failed writing PCAP section block pad");
        }
    }
    if (fwrite(&section.block_total_length,
               sizeof(section.block_total_length),
               1,
               m_fp_write)
        != 1) {
        fclose(m_fp_write);
        m_fp_write = nullptr;
        throw std::runtime_error("Failed writing PCAP section block");
    }

    pcap::interface_default_options interface_options{};

    interface_options.ts_resol.hdr.option_code =
        pcap::interface_option_type::IF_TSRESOL;
    interface_options.ts_resol.hdr.option_length = 1;
    interface_options.ts_resol.resolution = 9; // nano seconds
    interface_options.opt_end.hdr.option_code =
        pcap::interface_option_type::OPT_END;
    interface_options.opt_end.hdr.option_length = 0;

    pcap::interface_description_block interface_description;
    unpadded_block_length = sizeof(interface_description) + sizeof(uint32_t)
                            + sizeof(interface_options);
    block_length = pcap::pad_block_length(unpadded_block_length);
    pad_length = block_length - unpadded_block_length;

    interface_description.block_type = pcap::block_type::INTERFACE_DESCRIPTION;
    interface_description.block_total_length = block_length;
    interface_description.link_type = pcap::link_type::ETHERNET;
    interface_description.reserved = 0;
    interface_description.snap_len = 16384;

    if (fwrite(&interface_description,
               sizeof(interface_description),
               1,
               m_fp_write)
        != 1) {
        fclose(m_fp_write);
        m_fp_write = nullptr;
        throw std::runtime_error("Failed writing PCAP interface description");
    }
    if (fwrite(&interface_options, sizeof(interface_options), 1, m_fp_write)
        != 1) {
        fclose(m_fp_write);
        m_fp_write = nullptr;
        throw std::runtime_error(
            "Failed writing PCAP interface description options");
    }
    if (pad_length) {
        uint64_t pad_data = 0;
        if (fwrite(&pad_data, pad_length, 1, m_fp_write) != 1) {
            fclose(m_fp_write);
            m_fp_write = nullptr;
            throw std::runtime_error(
                "Failed writing PCAP interface description pad");
        }
    }
    if (fwrite(&interface_description.block_total_length,
               sizeof(interface_description.block_total_length),
               1,
               m_fp_write)
        != 1) {
        fclose(m_fp_write);
        m_fp_write = nullptr;
        throw std::runtime_error("Failed writing PCAP interface description");
    }
}

capture_buffer_file::~capture_buffer_file()
{
    if (m_fp_write) {
        fclose(m_fp_write);
        m_fp_write = nullptr;

        if (m_keep_file == keep_file::disabled) {
            if (std::filesystem::exists(m_filename)) {
                std::filesystem::remove(m_filename);
            }
        }
    }
}

uint16_t capture_buffer_file::write_packets(
    const openperf::packetio::packet::packet_buffer* const packets[],
    uint16_t packets_length)
{
    pcap::enhanced_packet_block block_hdr;
    block_hdr.block_type = pcap::block_type::ENHANCED_PACKET;
    block_hdr.interface_id = 0;

    pcap::enhanced_packet_block_default_options options;
    options.flags.hdr.option_code =
        pcap::enhanced_packet_block_option_type::FLAGS;
    options.flags.hdr.option_length = 4;
    options.flags.flags.value = 0;
    options.opt_end.hdr.option_code =
        pcap::enhanced_packet_block_option_type::OPT_END;
    options.opt_end.hdr.option_length = 0;

    if (m_full) { return 0; }
    for (uint16_t i = 0; i < packets_length; ++i) {
        auto packet = packets[i];
        auto packet_len = openperf::packetio::packet::length(packet);
        auto timestamp = openperf::packetio::packet::rx_timestamp(packet);

        // timestamp is set from interace description block
        // if_tsresol currently set to nanoseconds
        auto ts =
            std::chrono::time_point_cast<std::chrono::nanoseconds>(timestamp)
                .time_since_epoch()
                .count();

        block_hdr.packet_len = packet_len;
        block_hdr.captured_len =
            std::min(uint32_t(packet_len), m_max_packet_size);
        block_hdr.timestamp_high = (ts >> 32);
        block_hdr.timestamp_low = ts;

        if (openperf::packetio::packet::tx_sink(packet))
            options.flags.flags.set_direction(pcap::packet_direction::OUTBOUND);
        else
            options.flags.flags.set_direction(pcap::packet_direction::INBOUND);

        auto pad_length = pcap::pad_block_length(block_hdr.captured_len)
                          - block_hdr.captured_len;
        auto block_length = sizeof(block_hdr) + block_hdr.captured_len
                            + pad_length + sizeof(options) + sizeof(uint32_t);

        block_hdr.block_total_length = block_length;

        if (fwrite(&block_hdr, sizeof(block_hdr), 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR, "Failed writing enhanced packet block header");
            m_full = true;
            return i;
        }

        auto data = openperf::packetio::packet::to_data(packet);
        if (fwrite(data, block_hdr.captured_len, 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block data %" PRIu32,
                   block_hdr.captured_len);
            m_full = true;
            return i;
        }
        if (pad_length) {
            uint64_t pad_data = 0;
            if (fwrite(&pad_data, pad_length, 1, m_fp_write) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed writing enhanced packet block pad");
                m_full = true;
                return i;
            }
        }
        if (fwrite(&options, sizeof(options), 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block data options");
            m_full = true;
            return i;
        }
        if (fwrite(&block_hdr.block_total_length,
                   sizeof(block_hdr.block_total_length),
                   1,
                   m_fp_write)
            != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block trailing size");
            m_full = true;
            return i;
        }
        ++(m_stats.packets);
        m_stats.bytes += block_hdr.captured_len;
    }
    return packets_length;
}

std::unique_ptr<capture_buffer_reader> capture_buffer_file::create_reader()
{
    return std::unique_ptr<capture_buffer_reader>(
        new capture_buffer_file_reader(*this));
}

capture_buffer_stats capture_buffer_file::get_stats() const { return m_stats; }

void capture_buffer_file::flush()
{
    if (m_fp_write) { fflush(m_fp_write); }
}

///////////////////////////////////////////////////////////////////////////////

capture_buffer_file_reader::capture_buffer_file_reader(
    capture_buffer_file& buffer)
    : m_buffer(buffer)
    , m_fp_read(nullptr)
    , m_read_offset(0)
    , m_eof(false)
{
    auto filename = buffer.get_filename();
    buffer.flush();

    m_fp_read = fopen(filename.c_str(), "r");
    if (!m_fp_read) {
        throw std::runtime_error(
            std::string("Failed opening PCAP file for read.  ") + filename);
    }
    if (!read_file_header()) {
        fclose(m_fp_read);
        throw std::runtime_error(
            std::string("Failed reading PCAP file header.  ") + filename);
    }
}

capture_buffer_file_reader::~capture_buffer_file_reader()
{
    if (m_fp_read) {
        fclose(m_fp_read);
        m_fp_read = nullptr;
    }
}

bool capture_buffer_file_reader::read_file_header()
{
    ::rewind(m_fp_read);
    m_read_offset = 0;
    m_eof = false;

    // Read the pcap section header
    pcap::section_block section;
    if (fread(&section, sizeof(section), 1, m_fp_read) != 1) {
        OP_LOG(OP_LOG_ERROR, "Failed reading PCAP section block.");
        return false;
    }
    if (section.block_type != pcap::block_type::SECTION
        || section.byte_order_magic != pcap::BYTE_ORDER_MAGIC) {
        OP_LOG(OP_LOG_ERROR, "PCAP section not recognized.");
        return false;
    }
    auto remain = section.block_total_length - sizeof(section);
    if (remain > 0 && fseek(m_fp_read, remain, SEEK_CUR)) {
        OP_LOG(OP_LOG_ERROR, "Failed skipping PCAP section options.");
        return false;
    }

    m_read_offset = ftell(m_fp_read);

    // Read blocks until 1st packet
    while (true) {
        pcap::block_header block_header;
        if (fread(&block_header, sizeof(block_header), 1, m_fp_read) != 1) {
            // Capture file is valid but does not contain any packets
            return true;
        }
        if (block_header.block_type == pcap::block_type::ENHANCED_PACKET) {
            if (fseek(m_fp_read, m_read_offset, SEEK_SET)) {
                OP_LOG(OP_LOG_ERROR, "Failed setting PCAP read offset.");
                return false;
            }
            return true;
        }
        remain = block_header.block_length - sizeof(block_header);
        if (remain && fseek(m_fp_read, remain, SEEK_CUR)) {
            OP_LOG(OP_LOG_ERROR, "Failed skipping PCAP block data.");
            return false;
        }
        m_read_offset += block_header.block_length;
    }
    return false;
}

bool capture_buffer_file_reader::is_done() const { return m_eof; }

uint16_t capture_buffer_file_reader::read_packets(capture_packet* packets[],
                                                  uint16_t count)
{
    if (count > m_packets.size()) {
        // Allocate buffer space for all packets
        m_packets.resize(count);
        m_packet_data.resize(count * MAX_PACKET_SIZE);
    }

    uint8_t* data_ptr = &m_packet_data[0];
    pcap::enhanced_packet_block block_hdr;
    pcap::enhanced_packet_block_default_options options;

    uint16_t i = 0;
    while (i < count) {
        if (fread(&block_hdr, sizeof(block_hdr), 1, m_fp_read) != 1) {
            m_eof = true;
            break;
        }
        m_read_offset += sizeof(block_hdr);
        if (block_hdr.block_type != pcap::block_type::ENHANCED_PACKET) {
            OP_LOG(OP_LOG_ERROR,
                   "Unexepcted PCAP block type %" PRIu32,
                   block_hdr.block_type);
            m_eof = true;
            break;
        }

        auto block_remain = block_hdr.block_total_length - sizeof(block_hdr);
        size_t block_data_len =
            block_remain - sizeof(block_hdr.block_total_length);
        if (block_hdr.captured_len > block_data_len) {
            OP_LOG(OP_LOG_ERROR,
                   "PCAP block captured_len %" PRIu32
                   " is > block data length %zu",
                   block_hdr.captured_len,
                   block_data_len);
            block_hdr.captured_len = block_data_len;
        }

        uint64_t timestamp =
            (uint64_t)block_hdr.timestamp_high << 32 | block_hdr.timestamp_low;

        m_packets[i].hdr.timestamp =
            clock::time_point{std::chrono::nanoseconds{timestamp}};
        m_packets[i].hdr.captured_len = block_hdr.captured_len;
        m_packets[i].hdr.packet_len = block_hdr.packet_len;
        m_packets[i].data = data_ptr;
        m_packets[i].hdr.flags = 0;

        size_t pad_length = pcap::pad_block_length(block_hdr.captured_len)
                            - block_hdr.captured_len;

        // Data is too large to all fit in the buffer so read as much as
        // possible and skip over the rest
        if (block_hdr.captured_len > MAX_PACKET_SIZE) {
            // Not an error when just skipping trailing size bytes
            OP_LOG(OP_LOG_ERROR,
                   "Unexepcted PCAP block too large %" PRIu32,
                   block_hdr.block_total_length);
            block_hdr.captured_len = MAX_PACKET_SIZE;
        }
        if (fread(data_ptr, block_hdr.captured_len, 1, m_fp_read) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed reading PCAP enhanced packet block data %" PRIu32,
                   block_hdr.captured_len);
            m_eof = true;
            break;
        }
        m_read_offset += block_hdr.captured_len;
        block_remain -= block_hdr.captured_len;
        if (pad_length) {
            if (fseek(m_fp_read, pad_length, SEEK_CUR) != 0) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed skipping PCAP enhanced packet block data %zu",
                       pad_length);
                m_eof = true;
                break;
            }
            m_read_offset += pad_length;
            block_remain -= pad_length;
        }

        if (block_remain == sizeof(options)) {
            if (fread(&options, sizeof(options), 1, m_fp_read) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed reading PCAP enhanced packet block options %zu",
                       sizeof(options));
                m_eof = true;
                break;
            }
            m_packets[i].hdr.dir =
                static_cast<int>(options.flags.flags.get_direction());
        } else {
            if (fseek(m_fp_read, block_remain, SEEK_CUR) != 0) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed skipping PCAP enhanced packet block remaining "
                       "data %zu",
                       block_remain);
                m_eof = true;
                break;
            }
        }
        data_ptr += pad_capture_data_len(block_hdr.captured_len);
        packets[i] = &m_packets[i];
        ++i;
    }
    return i;
}

capture_buffer_stats capture_buffer_file_reader::get_stats() const
{
    return m_buffer.get_stats();
}

void capture_buffer_file_reader::rewind() { read_file_header(); }

///////////////////////////////////////////////////////////////////////////////

multi_capture_buffer_reader::multi_capture_buffer_reader(
    std::vector<std::unique_ptr<capture_buffer_reader>>&& readers)
    : m_reader_pending(nullptr)
{
    m_readers.reserve(readers.size());
    for (auto& reader : readers) {
        m_readers.emplace_back(burst_reader_type(std::move(reader)));
    }
    populate_timestamp_priority_list();
}

bool multi_capture_buffer_reader::is_done() const
{
    return m_reader_priority.empty() && (m_reader_pending == nullptr);
}

uint16_t multi_capture_buffer_reader::read_packets(capture_packet* packets[],
                                                   uint16_t count)
{
    if (m_reader_pending) {
        m_reader_pending->read_packets();
        if (!m_reader_pending->is_done()) {
            m_reader_priority.push(m_reader_pending);
        }
        m_reader_pending = nullptr;
    }

    if (m_reader_priority.empty()) return 0;

    auto reader = m_reader_priority.top();
    m_reader_priority.pop();

    auto last_timestamp = m_reader_priority.empty()
                              ? clock::time_point::max()
                              : m_reader_priority.top()->get()->hdr.timestamp;
    uint16_t i = 0;
    auto n = std::min(count, reader->m_packets.available());
    while (i < n) {
        auto packet = reader->get();
        if (packet->hdr.timestamp > last_timestamp) break;
        packets[i] = packet;
        ++i;
        ++(reader->m_packets.offset);
    }

    if (!reader->m_packets.is_empty()) {
        m_reader_priority.push(reader);
    } else {
        // Can't sort the reader without having packets, but can't get
        // new packets without invalidating stuff we may have returned.
        // So need to defer read_packets() until this function is called again.
        m_reader_pending = reader;
    }

    return i;
}

capture_buffer_stats multi_capture_buffer_reader::get_stats() const
{
    capture_buffer_stats total{0, 0};
    for (auto& reader : m_readers) {
        auto s = reader.get_stats();
        total.packets += s.packets;
        total.bytes += s.bytes;
    }
    return total;
}

void multi_capture_buffer_reader::rewind()
{
    m_reader_pending = nullptr;
    for (auto& reader : m_readers) { reader.rewind(); }
    populate_timestamp_priority_list();
}

size_t multi_capture_buffer_reader::get_offset() const
{
    size_t offset = 0;
    std::for_each(m_readers.begin(), m_readers.end(), [&](const auto& reader) {
        offset += reader.get_offset();
    });
    return offset;
}

void multi_capture_buffer_reader::populate_timestamp_priority_list()
{
    // Clear existing items
    while (!m_reader_priority.empty()) { m_reader_priority.pop(); }

    // Add all the readers
    for (auto& reader : m_readers) {
        if (reader.is_done()) continue;
        m_reader_priority.push(&reader);
    }
}

} // namespace openperf::packet::capture
