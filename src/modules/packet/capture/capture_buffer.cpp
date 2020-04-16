#include <cinttypes>
#include <cstdio>
#include <filesystem>

#include "core/op_log.h"
#include "packet/capture/capture_buffer.hpp"
#include "packet/capture/pcap_io.hpp"
#include "packetio/packet_buffer.hpp"

namespace openperf::packet::capture {

capture_buffer_file::capture_buffer_file(std::string_view filename,
                                         bool keep_file)
    : m_filename(filename)
    , m_keep_file(keep_file)
    , m_fp_write(nullptr)
    , m_stats{0, 0}
{
    m_fp_write = fopen(m_filename.c_str(), "w+");
    if (!m_fp_write) {
        throw std::runtime_error(
            std::string("Unable to open capture buffer file ") + m_filename);
    }

    pcapng::section_block section;

    auto unpadded_block_length = sizeof(section) + sizeof(uint32_t);
    auto block_length = pcapng::pad_block_length(unpadded_block_length);
    auto pad_length = block_length - unpadded_block_length;

    section.block_type = pcapng::block_type::SECTION;
    section.block_total_length = block_length;
    section.byte_order_magic = pcapng::BYTE_ORDER_MAGIC;
    section.major_version = 1;
    section.minor_version = 0;
    section.section_length = pcapng::SECTION_LENGTH_UNSPECIFIED;
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

    pcapng::interface_default_options interface_options;

    memset(&interface_options, 0, sizeof(interface_options));
    interface_options.ts_resol.hdr.option_code =
        pcapng::interface_option_type::IF_TSRESOL;
    interface_options.ts_resol.hdr.option_length = 1;
    interface_options.ts_resol.resolution = 9; // nano seconds
    interface_options.opt_end.hdr.option_code =
        pcapng::interface_option_type::OPT_END;
    interface_options.opt_end.hdr.option_length = 0;

    pcapng::interface_description_block interface_description;
    unpadded_block_length = sizeof(interface_description) + sizeof(uint32_t)
                            + sizeof(interface_options);
    block_length = pcapng::pad_block_length(unpadded_block_length);
    pad_length = block_length - unpadded_block_length;

    interface_description.block_type =
        pcapng::block_type::INTERFACE_DESCRIPTION;
    interface_description.block_total_length = block_length;
    interface_description.link_type = pcapng::link_type::ETHERNET;
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

capture_buffer_file::capture_buffer_file(capture_buffer_file&& rhs)
    : m_filename(rhs.m_filename)
    , m_keep_file(rhs.m_keep_file)
    , m_fp_write(rhs.m_fp_write)
    , m_stats(rhs.m_stats)
{
    rhs.m_fp_write = nullptr;
}

capture_buffer_file& capture_buffer_file::operator=(capture_buffer_file&& rhs)
{
    m_filename = rhs.m_filename;
    m_keep_file = rhs.m_keep_file;
    m_fp_write = rhs.m_fp_write;
    m_stats = rhs.m_stats;
    rhs.m_fp_write = nullptr;
    return *this;
}

int capture_buffer_file::push(
    const openperf::packetio::packets::packet_buffer* const packets[],
    uint16_t packets_length)
{
    pcapng::enhanced_packet_block block_hdr;
    block_hdr.block_type = pcapng::block_type::ENHANCED_PACKET;
    block_hdr.interface_id = 0;

    for (uint16_t i = 0; i < packets_length; ++i) {
        auto packet = packets[i];
        auto captured_len = openperf::packetio::packets::length(packet);
        auto timestamp = openperf::packetio::packets::rx_timestamp(packet);

        // timestamp is set from interace description block
        // if_tsresol currently set to nanoseconds
        auto ts =
            std::chrono::time_point_cast<std::chrono::nanoseconds>(timestamp)
                .time_since_epoch()
                .count();

        auto unpadded_block_length =
            sizeof(block_hdr) + captured_len + sizeof(uint32_t);
        auto block_length = pcapng::pad_block_length(unpadded_block_length);
        auto pad_length = block_length - unpadded_block_length;

        block_hdr.block_total_length = block_length;
        block_hdr.packet_len = captured_len; // FIXME:
        block_hdr.captured_len = captured_len;
        block_hdr.timestamp_high = (ts >> 32);
        block_hdr.timestamp_low = ts;
        if (fwrite(&block_hdr, sizeof(block_hdr), 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR, "Failed writing enhanced packet block header");
            return -1;
        }

        auto data = openperf::packetio::packets::to_data(packet);
        if (fwrite(data, captured_len, 1, m_fp_write) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed writing enhanced packet block data %" PRIu32,
                   captured_len);
            return -1;
        }
        if (pad_length) {
            uint64_t pad_data = 0;
            if (fwrite(&pad_data, pad_length, 1, m_fp_write) != 1) {
                OP_LOG(OP_LOG_ERROR,
                       "Failed writing enhanced packet block pad");
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
            return -1;
        }
        ++(m_stats.packets);
        m_stats.bytes += captured_len;
    }
    return 0;
}

capture_buffer_file::iterator capture_buffer_file::begin()
{
    fflush(m_fp_write);
    return capture_buffer_file::iterator(*this, false);
}

capture_buffer_file::iterator capture_buffer_file::end()
{
    return capture_buffer_file::iterator(*this, true);
}

buffer_stats capture_buffer_file::get_stats() { return m_stats; }

capture_buffer_file::iterator::iterator(capture_buffer_file& buffer, bool eof)
    : m_fp_read(nullptr)
    , m_read_offset(0)
    , m_eof(eof)
{
    if (!eof) {
        m_fp_read = fopen(buffer.m_filename.c_str(), "r");
        if (!m_fp_read) {
            throw std::runtime_error(
                std::string("Failed opening PCAP file for read.  ")
                + buffer.m_filename);
        }
        if (!read_file_header()) {
            fclose(m_fp_read);
            throw std::runtime_error(
                std::string("Failed reading PCAP file header.  ")
                + buffer.m_filename);
        }
        next();
    }
}

capture_buffer_file::iterator::iterator(capture_buffer_file::iterator&& rhs)
    : m_fp_read(rhs.m_fp_read)
    , m_read_offset(rhs.m_read_offset)
    , m_buffer_data(std::move(rhs.m_buffer_data))
    , m_read_buffer(std::move(rhs.m_read_buffer))
    , m_eof(rhs.m_eof)
{
    rhs.m_fp_read = nullptr;
    rhs.m_read_offset = 0;
    m_buffer_data.data = &m_read_buffer[0];
}

capture_buffer_file::iterator::~iterator()
{
    if (m_fp_read) {
        fclose(m_fp_read);
        m_fp_read = nullptr;
    }
}

capture_buffer_file::iterator& capture_buffer_file::iterator::
operator=(capture_buffer_file::iterator&& rhs)
{
    m_fp_read = rhs.m_fp_read;
    m_read_offset = rhs.m_read_offset;
    m_buffer_data = std::move(rhs.m_buffer_data);
    m_read_buffer = std::move(rhs.m_read_buffer);
    m_eof = rhs.m_eof;
    m_buffer_data.data = &m_read_buffer[0];

    rhs.m_fp_read = nullptr;
    rhs.m_read_offset = 0;

    return *this;
}

bool capture_buffer_file::iterator::read_file_header()
{
    rewind(m_fp_read);
    m_read_offset = 0;

    // Read the pcap section header
    pcapng::section_block section;
    if (fread(&section, sizeof(section), 1, m_fp_read) != 1) {
        OP_LOG(OP_LOG_ERROR, "Failed reading PCAP section block.");
        return false;
    }
    if (section.block_type != pcapng::block_type::SECTION
        || section.byte_order_magic != pcapng::BYTE_ORDER_MAGIC) {
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
        pcapng::block_header block_header;
        if (fread(&block_header, sizeof(block_header), 1, m_fp_read) != 1) {
            // Capture file is valid but does not contain any packets
            return true;
        }
        if (block_header.block_type == pcapng::block_type::ENHANCED_PACKET) {
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

bool capture_buffer_file::iterator::next()
{
    pcapng::enhanced_packet_block block_hdr;

    if (fread(&block_hdr, sizeof(block_hdr), 1, m_fp_read) != 1) {
        m_eof = true;
        return false;
    }
    m_read_offset += sizeof(block_hdr);
    if (block_hdr.block_type != pcapng::block_type::ENHANCED_PACKET) {
        OP_LOG(OP_LOG_ERROR,
               "Unexepcted PCAP block type %" PRIu32,
               block_hdr.block_type);
        m_eof = true;
        return false;
    }

    if (auto remain = block_hdr.block_total_length - sizeof(block_hdr)) {
        m_read_buffer.resize(remain);
        if (fread(&m_read_buffer[0], remain, 1, m_fp_read) != 1) {
            OP_LOG(OP_LOG_ERROR,
                   "Failed reading PCAP enhanced packet block data %zu",
                   remain);
            m_eof = true;
            return false;
        }
        m_read_offset += remain;
    }

    uint64_t timestamp =
        (uint64_t)block_hdr.timestamp_high << 32 | block_hdr.timestamp_low;

    m_buffer_data.timestamp = timestamp;
    m_buffer_data.captured_len = block_hdr.captured_len;
    m_buffer_data.packet_len = block_hdr.packet_len;
    m_buffer_data.data = &m_read_buffer[0];

    return true;
}

} // namespace openperf::packet::capture