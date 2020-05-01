#include "packet/generator/traffic/protocol/ethernet.hpp"
#include "packet/generator/traffic/protocol/vlan.hpp"

namespace openperf::packet::generator::traffic::protocol {

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::ipv4&) noexcept
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_ipv4);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::ipv6&) noexcept
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_ipv6);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::vlan&) noexcept
{
    /* If the vlan's  ether type is vlan, then our ether type should be qinq */
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_vlan);
}

void update_context(libpacket::protocol::vlan& vlan,
                    const libpacket::protocol::mpls&) noexcept
{
    set_vlan_ether_type(vlan, traffic::protocol::ethernet::ether_type_mpls);
}

flags update_packet_type(flags flags, const libpacket::protocol::vlan&) noexcept
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

header_lengths update_header_lengths(header_lengths& lengths,
                                     const libpacket::protocol::vlan&) noexcept
{
    lengths.layer2 += libpacket::protocol::vlan::protocol_length;
    return (lengths);
}

} // namespace openperf::packet::generator::traffic::protocol
