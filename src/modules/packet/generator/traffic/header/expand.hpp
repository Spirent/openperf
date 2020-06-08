#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_HPP_

#include "packet/generator/traffic/header/config.hpp"
#include "packet/generator/traffic/header/container.hpp"

namespace openperf::packet::generator::traffic::header {

container expand(const ethernet_config&);
container expand(const mpls_config&);
container expand(const vlan_config&);

container expand(const ipv4_config&);
container expand(const ipv6_config&);

container expand(const tcp_config&);
container expand(const udp_config&);

container expand(const custom_config&);

} // namespace openperf::packet::generator::traffic::header

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_HEADER_EXPAND_HPP_ */
