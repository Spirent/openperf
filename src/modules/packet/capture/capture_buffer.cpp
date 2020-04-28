#include <cinttypes>
#include <cstdio>
#include <filesystem>

#include "core/op_log.h"
#include "packet/capture/capture_buffer.hpp"
#include "packet/capture/pcap_defs.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::capture {

constexpr size_t max_packet_size = 16384;

capture_buffer_mem::capture_buffer_mem(size_t size)
    : m_mem_size(size)
    , m_start_addr(nullptr)
    , m_end_addr(nullptr)
    , m_cur_addr(nullptr)
    , m_stats{0, 0}
    , m_full(false)
{
    m_mem = std::make_unique<uint8_t[]>(m_mem_size);
    m_start_addr = m_mem.get();
    m_end_addr = m_mem.get() + m_mem_size;
    m_cur_addr = m_start_addr;
}

capture_buffer_mem::~capture_buffer_mem() {}

int capture_buffer_mem::write_packets(
    const openperf::packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length)
{
    capture_packet_hdr hdr;
    for (uint16_t i = 0; i < packets_length; ++i) {
        auto& packet = packets[i];
        auto timestamp = openperf::packetio::packets::rx_timestamp(packet);
        auto data = openperf::packetio::packets::to_data(packet);
        hdr.packet_len = openperf::packetio::packets::length(packet);
        hdr.captured_len =
            hdr.packet_len; // TODO: Currently capturing all of packet data
        hdr.timestamp =
            std::chrono::time_point_cast<std::chrono::nanoseconds>(timestamp)
                .time_since_epoch()
                .count();
        auto padded_data_len = pad_capture_data_len(hdr.captured_len);
        if (m_cur_addr + sizeof(hdr) + padded_data_len > m_end_addr) {
            m_full = true;
            return -1;
        }
        *reinterpret_cast<capture_packet_hdr*>(m_cur_addr) = hdr;
        m_cur_addr += sizeof(hdr);
        std::copy(reinterpret_cast<const uint8_t*>(data),
                  reinterpret_cast<const uint8_t*>(data) + hdr.captured_len,
                  m_cur_addr);
        m_cur_addr += padded_data_len;
        m_stats.packets += 1;
        m_stats.bytes += hdr.captured_len;
    }

    return 0;
}

std::unique_ptr<capture_buffer_reader> capture_buffer_mem::create_reader()
{
    return std::make_unique<capture_buffer_mem_reader>(*this);
}

capture_buffer_stats capture_buffer_mem::get_stats() const { return m_stats; }

///////////////////////////////////////////////////////////////////////////////

capture_buffer_mem_reader::capture_buffer_mem_reader(capture_buffer_mem& buffer)
    : m_buffer(buffer)
    , m_cur_addr(nullptr)
    , m_end_addr(nullptr)
    , m_read_offset(0)
    , m_eof(false)
{
    rewind();
}

capture_buffer_mem_reader::~capture_buffer_mem_reader() {}

bool capture_buffer_mem_reader::is_done() const { return m_eof; }

size_t capture_buffer_mem_reader::read_packets(capture_packet* packets[],
                                               size_t count)
{
    if (count > m_packets.size()) {
        // Allocate buffer space for all packets
        m_packets.resize(count);
    }

    size_t i = 0;
    while (i < count) {
        auto& packet = m_packets[i];
        if (m_cur_addr + sizeof(packet.hdr) > m_end_addr) {
            m_eof = true;
            break;
        }
        packet.hdr = *reinterpret_cast<capture_packet_hdr*>(m_cur_addr);
        m_cur_addr += sizeof(packet.hdr);
        if (m_cur_addr + packet.hdr.captured_len > m_end_addr) {
            m_eof = true;
            break;
        }
        packet.data = m_cur_addr;
        m_cur_addr += pad_capture_data_len(packet.hdr.captured_len);
        packets[i] = &packet;
        ++i;
    }
    return i;
}

capture_buffer_stats capture_buffer_mem_reader::get_stats() const
{
    return m_buffer.get_stats();
}

void capture_buffer_mem_reader::rewind()
{
    m_read_offset = 0;
    m_cur_addr = m_buffer.get_start_addr();
    m_end_addr = m_buffer.get_cur_addr();
    m_eof = false;
}

///////////////////////////////////////////////////////////////////////////////

capture_buffer_file::capture_buffer_file(std::string_view filename,
                                         bool keep_file)
    : m_filename(filename)
    , m_keep_file(keep_file)
    , m_full(false)
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

    pcap::interface_default_options interface_options;

    memset(&interface_options, 0, sizeof(interface_options));
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

        if (!m_keep_file) {
            if (std::filesystem::exists(m_filename)) {
                std::filesystem::remove(m_filename);
            }
        }
    }
}

int capture_buffer_file::write_packets(
    const openperf::packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length)
{
    pcap::enhanced_packet_block block_hdr;
    block_hdr.block_type = pcap::block_type::ENHANCED_PACKET;
    block_hdr.interface_id = 0;

    for (uint16_t i = 0; i < packets_length; ++i) {
        auto packet = packets[i];
        auto packet_len = openperf::packetio::packets::length(packet);
        auto timestamp = openperf::packetio::packets::rx_timestamp(packet);

        // timestamp is set from interace description block
        // if_tsresol currently set to nanoseconds
        auto ts =
            std::chrono::time_point_cast<std::chrono::nanoseconds>(timestamp)
                .time_since_epoch()
                .count();

        block_hdr.packet_len = packet_len;
        block_hdr.captured_len =
            packet_len; // TODO: Currently capturing all of packet data
        block_hdr.timestamp_high = (ts >> 32);
        block_hdr.timestamp_low = ts;

        auto unpadded_block_length =
            sizeof(block_hdr) + block_hdr.captured_len + sizeof(uint32_t);
        auto block_length = pcap::pad_block_length(unpadded_block_length);
        auto pad_length = block_length - unpadded_block_length;

        block_hdr.block_total_length = block_length;

        if (fwrite(&block_hdr, sizeof(block_hdr), 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR, "Failed writing enhanced packet block header");
            m_full = true;
            return -1;
        }

        auto data = openperf::packetio::packets::to_data(packet);
        if (fwrite(data, block_hdr.captured_len, 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block data %" PRIu32,
                   block_hdr.captured_len);
            m_full = true;
            return -1;
        }
        if (pad_length) {
            uint64_t pad_data = 0;
            if (fwrite(&pad_data, pad_length, 1, m_fp_write) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed writing enhanced packet block pad");
                m_full = true;
                return -1;
            }
        }
        if (fwrite(&block_hdr.block_total_length,
                   sizeof(block_hdr.block_total_length),
                   1,
                   m_fp_write)
            != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block trailing size");
            m_full = true;
            return -1;
        }
        ++(m_stats.packets);
        m_stats.bytes += block_hdr.captured_len;
    }
    return 0;
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

size_t capture_buffer_file_reader::read_packets(capture_packet* packets[],
                                                size_t count)
{
    if (count > m_packets.size()) {
        // Allocate buffer space for all packets
        m_packets.resize(count);
        m_packet_data.resize(count * max_packet_size);
    }

    uint8_t* data_ptr = &m_packet_data[0];
    pcap::enhanced_packet_block block_hdr;
    size_t i = 0;
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

        m_packets[i].hdr.timestamp = timestamp;
        m_packets[i].hdr.captured_len = block_hdr.captured_len;
        m_packets[i].hdr.packet_len = block_hdr.packet_len;
        m_packets[i].data = data_ptr;

        if (block_remain < max_packet_size) {
            if (fread(data_ptr, block_remain, 1, m_fp_read) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed reading PCAP enhanced packet block data %zu",
                       block_remain);
                m_eof = true;
                break;
            }
            m_read_offset += block_remain;
        } else {
            if (block_hdr.captured_len > max_packet_size) {
                OP_LOG(OP_LOG_ERROR,
                       "Unexepcted PCAP block too large %" PRIu32,
                       block_hdr.block_total_length);
                block_hdr.captured_len = max_packet_size;
            }
            if (fread(data_ptr, max_packet_size, 1, m_fp_read) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed reading PCAP enhanced packet block data %zu",
                       max_packet_size);
                m_eof = true;
                break;
            }
            m_read_offset += max_packet_size;
            block_remain -= max_packet_size;
            if (fseek(m_fp_read, block_remain, SEEK_CUR) != 0) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed skipping PCAP enhanced packet block data %zu",
                       block_remain);
                m_eof = true;
                break;
            }
            m_read_offset += block_remain;
        }
        data_ptr += pcap::pad_block_length(block_hdr.captured_len);
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

size_t multi_capture_buffer_reader::read_packets(capture_packet* packets[],
                                                 size_t count)
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

    uint64_t last_timestamp =
        m_reader_priority.empty()
            ? UINT64_MAX
            : m_reader_priority.top()->get()->hdr.timestamp;
    size_t i = 0;
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
