
#include "core/op_core.h"
#include "packet/capture/pcap_writer.hpp"
#include "packet/capture/capture_buffer.hpp"

namespace openperf::packet::capture::pcap {

bool pcap_buffer_writer::write_section_block()
{
    uint8_t* ptr = m_buffer.data();
    section_block section;
    auto unpadded_block_length = sizeof(section) + sizeof(uint32_t);
    auto block_length = pad_block_length(unpadded_block_length);

    if (block_length > get_available_length()) return false;

    section.block_type = block_type::SECTION;
    section.block_total_length = block_length;
    section.byte_order_magic = BYTE_ORDER_MAGIC;
    section.major_version = 1;
    section.minor_version = 0;
    section.section_length = SECTION_LENGTH_UNSPECIFIED;
    std::copy_n(reinterpret_cast<uint8_t*>(&section),
                sizeof(section),
                ptr + m_buffer_length);
    m_buffer_length += section.block_total_length;
    std::copy_n(reinterpret_cast<uint8_t*>(&section.block_total_length),
                sizeof(section.block_total_length),
                ptr + m_buffer_length - sizeof(section.block_total_length));

    return true;
}

bool pcap_buffer_writer::write_interface_block()
{
    uint8_t* ptr = m_buffer.data();
    interface_default_options interface_options;

    memset(&interface_options, 0, sizeof(interface_options));
    interface_options.ts_resol.hdr.option_code =
        interface_option_type::IF_TSRESOL;
    interface_options.ts_resol.hdr.option_length = 1;
    interface_options.ts_resol.resolution = 9; // nano seconds
    interface_options.opt_end.hdr.option_code = interface_option_type::OPT_END;
    interface_options.opt_end.hdr.option_length = 0;

    interface_description_block interface_description;
    auto unpadded_block_length = sizeof(interface_description)
                                 + sizeof(uint32_t) + sizeof(interface_options);
    auto block_length = pad_block_length(unpadded_block_length);

    if (block_length > get_available_length()) return false;

    interface_description.block_type = block_type::INTERFACE_DESCRIPTION;
    interface_description.block_total_length = block_length;
    interface_description.link_type = link_type::ETHERNET;
    interface_description.reserved = 0;
    interface_description.snap_len = 16384;

    std::copy_n(reinterpret_cast<uint8_t*>(&interface_description),
                sizeof(interface_description),
                ptr + m_buffer_length);
    std::copy_n(reinterpret_cast<uint8_t*>(&interface_options),
                sizeof(interface_options),
                ptr + m_buffer_length + sizeof(interface_description));
    m_buffer_length += interface_description.block_total_length;
    std::copy_n(
        reinterpret_cast<uint8_t*>(&interface_description.block_total_length),
        sizeof(interface_description.block_total_length),
        ptr + m_buffer_length
            - sizeof(interface_description.block_total_length));

    return true;
}

bool pcap_buffer_writer::write_file_header()
{
    m_buffer_length = 0;

    if (!write_section_block()) { return false; }

    if (!write_interface_block()) { return false; }

    return true;
}

bool pcap_buffer_writer::write_packet(const capture_packet& packet)
{
    enhanced_packet_block block_hdr;
    pcap::enhanced_packet_block_default_options options;

    auto timestamp = std::chrono::time_point_cast<std::chrono::nanoseconds>(
                         packet.hdr.timestamp)
                         .time_since_epoch()
                         .count();

    block_hdr.block_type = block_type::ENHANCED_PACKET;
    block_hdr.interface_id = 0;
    block_hdr.timestamp_high = timestamp >> 32;
    block_hdr.timestamp_low = timestamp;
    block_hdr.captured_len = packet.hdr.captured_len;
    block_hdr.packet_len = packet.hdr.packet_len;
    block_hdr.block_total_length =
        sizeof(block_hdr) + pad_block_length(packet.hdr.captured_len)
        + sizeof(options) + sizeof(block_hdr.block_total_length);

    options.flags.hdr.option_code =
        pcap::enhanced_packet_block_option_type::FLAGS;
    options.flags.hdr.option_length = 4;
    options.flags.flags.value = 0;
    options.opt_end.hdr.option_code =
        pcap::enhanced_packet_block_option_type::OPT_END;
    options.opt_end.hdr.option_length = 0;
    if (packet.hdr.flags & CAPTURE_FLAG_TX)
        options.flags.flags.set_direction(pcap::packet_direction::OUTBOUND);
    else
        options.flags.flags.set_direction(pcap::packet_direction::INBOUND);

    return write_packet_block(
        block_hdr, packet.data, &options, sizeof(options));
}

bool pcap_buffer_writer::write_packet_block(
    const enhanced_packet_block& block_hdr,
    const uint8_t* block_data,
    const void* options_data,
    size_t options_length)
{
    if (block_hdr.block_total_length > get_available_length()) { return false; }

    auto ptr = m_buffer.data() + m_buffer_length;
    auto pad_len =
        pad_block_length(block_hdr.captured_len) - block_hdr.captured_len;

    // Copy block header
    std::copy_n(
        reinterpret_cast<const uint8_t*>(&block_hdr), sizeof(block_hdr), ptr);
    ptr += sizeof(block_hdr);

    // Copy packet data
    std::copy_n(block_data, block_hdr.captured_len, ptr);
    ptr += block_hdr.captured_len;

    // Skip pad bytes
    if (pad_len) { ptr += pad_len; }

    // Copy options data
    std::copy_n(
        reinterpret_cast<const uint8_t*>(options_data), options_length, ptr);
    ptr += options_length;

    // Copy block total length trailer
    std::copy_n(reinterpret_cast<const uint8_t*>(&block_hdr.block_total_length),
                sizeof(block_hdr.block_total_length),
                ptr);
    ptr += sizeof(block_hdr.block_total_length);
    m_buffer_length += block_hdr.block_total_length;
    assert(m_buffer.data() + m_buffer_length == ptr);
    return true;
}

} // namespace openperf::packet::capture::pcap
