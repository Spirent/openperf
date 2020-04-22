#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPLODE_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPLODE_HPP_

#include <vector>

#include "packet/generator/traffic/header/config.hpp"
#include "packet/generator/traffic/header/container.hpp"

namespace openperf::packet::generator::traffic::header {

container explode(const std::vector<container>& headers, modifier_mux mux);

} // namespace openperf::packet::generator::traffic::header

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPLODE_HPP_ */
