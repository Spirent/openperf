
#include "core/op_core.h"
#include "packet/capture/pcap_writer.hpp"
#include "packet/capture/pcap_io.hpp"
#include "packet/capture/capture_buffer.hpp"

namespace openperf::packet::capture {

pcap_buffered_writer::pcap_buffered_writer() {}

size_t pcap_buffered_writer::calc_length(capture_buffer_reader& reader)
{
    size_t header_length =
        pcapng::pad_block_length(sizeof(pcapng::section_block)
                                 + sizeof(uint32_t))
        + pcapng::pad_block_length(sizeof(pcapng::interface_description_block)
                                   + sizeof(uint32_t)
                                   + sizeof(pcapng::interface_default_options));
    size_t length = header_length;
    reader.rewind();
    reader.read([&](auto& buffer) {
        length += pcapng::pad_block_length(sizeof(pcapng::enhanced_packet_block)
                                           + sizeof(uint32_t)
                                           + buffer.hdr.captured_len);
        return true;
    });
    reader.rewind();

    return length;
}

bool pcap_buffered_writer::write_section_block()
{
    uint8_t* ptr = m_buffer.data();
    pcapng::section_block section;
    auto unpadded_block_length = sizeof(section) + sizeof(uint32_t);
    auto block_length = pcapng::pad_block_length(unpadded_block_length);

    if (block_length > get_available_length()) return false;

    section.block_type = pcapng::block_type::SECTION;
    section.block_total_length = block_length;
    section.byte_order_magic = pcapng::BYTE_ORDER_MAGIC;
    section.major_version = 1;
    section.minor_version = 0;
    section.section_length = pcapng::SECTION_LENGTH_UNSPECIFIED;
    std::copy(reinterpret_cast<uint8_t*>(&section),
              reinterpret_cast<uint8_t*>(&section) + sizeof(section),
              ptr + m_buffer_length);
    m_buffer_length += section.block_total_length;
    std::copy(reinterpret_cast<uint8_t*>(&section.block_total_length),
              reinterpret_cast<uint8_t*>(&section.block_total_length)
                  + sizeof(section.block_total_length),
              ptr + m_buffer_length - sizeof(section.block_total_length));

    return true;
}

bool pcap_buffered_writer::write_interface_block()
{
    uint8_t* ptr = m_buffer.data();
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
    auto unpadded_block_length = sizeof(interface_description)
                                 + sizeof(uint32_t) + sizeof(interface_options);
    auto block_length = pcapng::pad_block_length(unpadded_block_length);

    if (block_length > get_available_length()) return false;

    interface_description.block_type =
        pcapng::block_type::INTERFACE_DESCRIPTION;
    interface_description.block_total_length = block_length;
    interface_description.link_type = pcapng::link_type::ETHERNET;
    interface_description.reserved = 0;
    interface_description.snap_len = 16384;

    std::copy(reinterpret_cast<uint8_t*>(&interface_description),
              reinterpret_cast<uint8_t*>(&interface_description)
                  + sizeof(interface_description),
              ptr + m_buffer_length);
    std::copy(reinterpret_cast<uint8_t*>(&interface_options),
              reinterpret_cast<uint8_t*>(&interface_options)
                  + sizeof(interface_options),
              ptr + m_buffer_length + sizeof(interface_description));
    m_buffer_length += interface_description.block_total_length;
    std::copy(
        reinterpret_cast<uint8_t*>(&interface_description.block_total_length),
        reinterpret_cast<uint8_t*>(&interface_description.block_total_length)
            + sizeof(interface_description.block_total_length),
        ptr + m_buffer_length
            - sizeof(interface_description.block_total_length));

    return true;
}

bool pcap_buffered_writer::write_start()
{
    m_buffer_length = 0;

    if (!write_section_block()) { return false; }

    if (!write_interface_block()) { return false; }

    return true;
}

bool pcap_buffered_writer::write_packet(const capture_packet& packet)
{
    pcapng::enhanced_packet_block block_hdr;

    block_hdr.block_type = pcapng::block_type::ENHANCED_PACKET;
    block_hdr.interface_id = 0;
    block_hdr.timestamp_high = packet.hdr.timestamp >> 32;
    block_hdr.timestamp_low = packet.hdr.timestamp;
    block_hdr.captured_len = packet.hdr.captured_len;
    block_hdr.packet_len = packet.hdr.packet_len;
    block_hdr.block_total_length = pcapng::pad_block_length(
        sizeof(block_hdr) + sizeof(uint32_t) + packet.hdr.captured_len);

    return write_packet_block(block_hdr, packet.data);
}

bool pcap_buffered_writer::write_packet_block(
    const pcapng::enhanced_packet_block& block_hdr, const uint8_t* block_data)
{
    uint8_t* ptr = m_buffer.data();

    if (block_hdr.block_total_length > get_available_length()) { return false; }

    std::copy(reinterpret_cast<const uint8_t*>(&block_hdr),
              reinterpret_cast<const uint8_t*>(&block_hdr) + sizeof(block_hdr),
              ptr + m_buffer_length);
    std::copy(block_data,
              block_data + block_hdr.captured_len,
              ptr + m_buffer_length + sizeof(block_hdr));
    m_buffer_length += block_hdr.block_total_length;
    std::copy(reinterpret_cast<const uint8_t*>(&block_hdr.block_total_length),
              reinterpret_cast<const uint8_t*>(&block_hdr.block_total_length)
                  + sizeof(block_hdr.block_total_length),
              ptr + m_buffer_length - sizeof(block_hdr.block_total_length));
    return true;
}

} // namespace openperf::packet::capture