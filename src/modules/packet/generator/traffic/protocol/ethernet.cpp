#include "packet/generator/traffic/protocol/ethernet.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::ethernet& eth,
                    const libpacket::protocol::ipv4&)
{
    set_ethernet_ether_type(eth, traffic::protocol::ethernet::ether_type_ipv4);
}

void update_context(libpacket::protocol::ethernet& eth,
                    const libpacket::protocol::ipv6&)
{
    set_ethernet_ether_type(eth, traffic::protocol::ethernet::ether_type_ipv6);
}

void update_context(libpacket::protocol::ethernet& eth,
                    const libpacket::protocol::vlan& vlan)
{
    /* If the vlan's ether type is vlan, then our ether type should be QinQ */
    set_ethernet_ether_type(
        eth,
        (get_vlan_ether_type(vlan)
                 == traffic::protocol::ethernet::ether_type_vlan
             ? traffic::protocol::ethernet::ether_type_qinq
             : traffic::protocol::ethernet::ether_type_vlan));
}

void update_context(libpacket::protocol::ethernet& eth,
                    const libpacket::protocol::mpls&)
{
    set_ethernet_ether_type(eth, traffic::protocol::ethernet::ether_type_mpls);
}

flags update_packet_type(flags flags, const libpacket::protocol::ethernet&)
{
    return (flags | packetio::packet::packet_type::ethernet::ether);
}

void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::ethernet&)
{
    lengths.layer2 += libpacket::protocol::ethernet::protocol_length;
}

} // namespace openperf::packet::generator::traffic::protocol
