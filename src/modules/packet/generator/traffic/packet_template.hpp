#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_

#include "packet/generator/traffic/header/container.hpp"
#include "packet/generator/traffic/header/utils.hpp"
#include "packet/generator/traffic/view_iterator.hpp"

#include "packetio/packet_buffer.hpp"

namespace openperf::packet::generator::traffic {

class packet_template
{
    header::container m_headers;
    packetio::packet::header_lengths m_hdr_lens;
    packetio::packet::packet_type::flags m_flags;

public:
    using view_type = const uint8_t*;
    using iterator = view_iterator<packet_template>;
    using const_iterator = const iterator;

    packet_template(const header::config_container& configs,
                    header::modifier_mux mux);

    packetio::packet::header_lengths header_lengths() const;
    packetio::packet::packet_type::flags header_flags() const;

    size_t size() const;

    view_type operator[](size_t idx) const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;
};

void update_packet_header_lengths(
    const uint8_t header[],
    packetio::packet::header_lengths hdr_lens,
    packetio::packet::packet_type::flags hdr_flags,
    uint16_t pkt_len,
    uint8_t packet[]);

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_ */
