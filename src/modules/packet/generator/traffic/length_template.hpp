#ifndef _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_TEMPLATE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_TEMPLATE_HPP_

#include <variant>
#include <vector>

#include "packet/generator/traffic/length_config.hpp"
#include "packet/generator/traffic/view_iterator.hpp"

namespace openperf::packet::generator::traffic {

class length_template
{
public:
    using view_type = uint16_t;
    using iterator = view_iterator<length_template>;
    using const_iterator = const iterator;

    length_template(length_container&& lengths);

    uint16_t max_packet_length() const;

    size_t size() const;

    view_type operator[](size_t idx) const;

    iterator begin();
    const_iterator begin() const;

    iterator end();
    const_iterator end() const;

private:
    length_container m_lengths;
};

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_TEMPLATE_HPP_ */
