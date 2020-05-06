#ifndef _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_HPP_

#include "packet/generator/traffic/modifier.hpp"

namespace openperf::packet::generator::traffic {

struct length_fixed_config
{
    uint16_t length;
};

using length_list_config = modifier::list_config<uint16_t>;
using length_sequence_config = modifier::sequence_config<uint16_t>;

using length_config = std::
    variant<length_fixed_config, length_list_config, length_sequence_config>;

using length_container = std::vector<uint16_t>;

length_container get_lengths(const length_config&);
length_container get_lengths(length_config&&);

} // namespace openperf::packet::generator::traffic

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_LENGTH_HPP_ */
