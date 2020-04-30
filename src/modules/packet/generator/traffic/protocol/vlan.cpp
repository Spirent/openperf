#include "packet/generator/traffic/protocol/ethernet.hpp"
#include "packet/generator/traffic/protocol/vlan.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::ipv4&)
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_ipv4);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::ipv6&)
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_ipv6);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::vlan&)
{
    /* If the vlan's  ether type is vlan, then our ether type should be qinq */
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_vlan);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::mpls&)
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_mpls);
}

flags update_packet_type(flags flags, const libpacket::protocol::vlan&)
{
    using namespace openperf::packetio::packet;

    switch (packet_type::ethernet(
        (flags & packetio::packet::packet_type::ethernet::mask).value)) {
    case packet_type::ethernet::vlan:
        return ((flags & ~packet_type::ethernet::mask)
                | packet_type::ethernet::qinq);
    default:
        return ((flags & ~packet_type::ethernet::mask)
                | packet_type::ethernet::vlan);
    }
}

void update_header_lengths(header_lengths& lengths,
                           const libpacket::protocol::vlan&)
{
    lengths.layer2 += libpacket::protocol::vlan::protocol_length;
}

} // namespace openperf::packet::generator::traffic::protocol
