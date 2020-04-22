#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_

#include "packet/generator/traffic/header/container.hpp"
#include "packet/generator/traffic/header/utils.hpp"
#include "packet/generator/traffic/view_iterator.hpp"

namespace openperf::packet::generator::traffic {

class packet_template
{
    header::container m_headers;
    std::vector<uint16_t> m_lengths;
    header::config_key m_key;

public:
    struct header_descriptor
    {
        const header::config_key& key;
        const uint8_t* ptr;
        uint16_t len;
    };

    using view_type = std::pair<header_descriptor, uint16_t>;
    using iterator = view_iterator<packet_template>;
    using const_iterator = const iterator;

    packet_template(const header::config_container& configs,
                    header::modifier_mux mux,
                    const std::vector<uint16_t> lengths);

    uint16_t max_packet_length() const;

    size_t size() const;

    view_type operator[](size_t idx) const;

    bool operator==(const packet_template& other) const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PACKET_TEMPLATE_HPP_ */
