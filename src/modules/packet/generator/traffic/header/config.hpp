#ifndef _OP_PACKET_GENERATOR_TRAFFIC_HEADER_CONFIG_HPP_
#define _OP_PACKET_GENERATOR_TRAFFIC_HEADER_CONFIG_HPP_

#include <vector>

#include "packet/protocol/protocols.hpp"
#include "packet/generator/traffic/modifier.hpp"

namespace openperf::packet::generator::traffic::header {

enum class modifier_mux { none = 0, zip, cartesian };

template <typename PacketHeader> struct config
{
    using field_name = typename PacketHeader::field_name;
    using modifier_key_value = std::pair<field_name, modifier::config>;
    using modifier_container = std::vector<modifier_key_value>;

    PacketHeader header;
    modifier_container modifiers;
    modifier_mux mux = modifier_mux::none;
};

using ethernet_config = config<libpacket::protocol::ethernet>;
using mpls_config = config<libpacket::protocol::mpls>;
using vlan_config = config<libpacket::protocol::vlan>;

using ipv4_config = config<libpacket::protocol::ipv4>;
using ipv6_config = config<libpacket::protocol::ipv6>;

using tcp_config = config<libpacket::protocol::tcp>;
using udp_config = config<libpacket::protocol::udp>;

using config_instance = std::variant<ethernet_config,
                                     mpls_config,
                                     vlan_config,
                                     ipv4_config,
                                     ipv6_config,
                                     tcp_config,
                                     udp_config>;

using config_container = std::vector<config_instance>;

} // namespace openperf::packet::generator::traffic::header

#endif /* _OP_PACKET_GENERATOR_TRAFFIC_HEADER_CONFIG_HPP_ */
