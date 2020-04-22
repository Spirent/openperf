#ifndef _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_

#include <cstdint>

#include "packet/protocol/protocols.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv4&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::ipv6&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::vlan&);
void update_context(libpacket::protocol::vlan&,
                    const libpacket::protocol::mpls&);

} // namespace openperf::packet::generator::traffic::protocol

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_PROTOCOL_VLAN_HPP_ */
