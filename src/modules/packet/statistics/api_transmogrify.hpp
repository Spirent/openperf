#ifndef _OP_PACKET_STATISTICS_API_TRANSMOGRIFY_HPP_
#define _OP_PACKET_STATISTICS_API_TRANSMOGRIFY_HPP_

#include "packet/statistics/generic_protocol_counters.hpp"

#include "swagger/v1/model/PacketEthernetProtocolCounters.h"
#include "swagger/v1/model/PacketIpProtocolCounters.h"
#include "swagger/v1/model/PacketTransportProtocolCounters.h"
#include "swagger/v1/model/PacketTunnelProtocolCounters.h"
#include "swagger/v1/model/PacketInnerEthernetProtocolCounters.h"
#include "swagger/v1/model/PacketInnerIpProtocolCounters.h"
#include "swagger/v1/model/PacketInnerTransportProtocolCounters.h"

namespace openperf::packet::statistics::api {

template <typename T>
void populate_counters(
    const openperf::packet::statistics::generic_protocol_counters& src, T& dst)
{
    using namespace openperf::packet::statistics::protocol;

    if (src.holds<ethernet>()) {
        const auto& p_src = src.get<ethernet>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketEthernetProtocolCounters>();

        p_dst->setIp(p_src[ethernet::index::ether]);
        p_dst->setTimesync(p_src[ethernet::index::timesync]);
        p_dst->setArp(p_src[ethernet::index::arp]);
        p_dst->setLldp(p_src[ethernet::index::lldp]);
        p_dst->setNsh(p_src[ethernet::index::nsh]);
        p_dst->setVlan(p_src[ethernet::index::vlan]);
        p_dst->setQinq(p_src[ethernet::index::qinq]);
        p_dst->setPppoe(p_src[ethernet::index::pppoe]);
        p_dst->setFcoe(p_src[ethernet::index::fcoe]);
        p_dst->setMpls(p_src[ethernet::index::mpls]);

        dst->setEthernet(p_dst);
    }
    if (src.holds<ip>()) {
        const auto& p_src = src.get<ip>();
        auto p_dst =
            std::make_shared<swagger::v1::model::PacketIpProtocolCounters>();

        p_dst->setIpv4(p_src[ip::index::ipv4] + p_src[ip::index::ipv4_ext]
                       + p_src[ip::index::ipv4_ext_unknown]);
        p_dst->setIpv6(p_src[ip::index::ipv6] + p_src[ip::index::ipv6_ext]
                       + p_src[ip::index::ipv6_ext_unknown]);

        dst->setIp(p_dst);
    }
    if (src.holds<transport>()) {
        const auto& p_src = src.get<transport>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketTransportProtocolCounters>();

        p_dst->setTcp(p_src[transport::index::tcp]);
        p_dst->setUdp(p_src[transport::index::udp]);
        p_dst->setFragmented(p_src[transport::index::fragment]);
        p_dst->setSctp(p_src[transport::index::sctp]);
        p_dst->setIcmp(p_src[transport::index::icmp]);
        p_dst->setNonFragmented(p_src[transport::index::non_fragment]);
        p_dst->setIgmp(p_src[transport::index::igmp]);

        dst->setTransport(p_dst);
    }
    if (src.holds<tunnel>()) {
        const auto& p_src = src.get<tunnel>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketTunnelProtocolCounters>();

        p_dst->setIp(p_src[tunnel::index::ip]);
        p_dst->setGre(p_src[tunnel::index::gre]);
        p_dst->setVxlan(p_src[tunnel::index::vxlan]);
        p_dst->setNvgre(p_src[tunnel::index::nvgre]);
        p_dst->setGeneve(p_src[tunnel::index::geneve]);
        p_dst->setGrenat(p_src[tunnel::index::grenat]);
        p_dst->setGtpc(p_src[tunnel::index::gtpc]);
        p_dst->setGtpu(p_src[tunnel::index::gtpu]);
        p_dst->setEsp(p_src[tunnel::index::esp]);
        p_dst->setL2tp(p_src[tunnel::index::l2tp]);
        p_dst->setVxlanGpe(p_src[tunnel::index::vxlan_gpe]);
        p_dst->setMplsInGre(p_src[tunnel::index::mpls_in_gre]);
        p_dst->setMplsInUdp(p_src[tunnel::index::mpls_in_udp]);

        dst->setTunnel(p_dst);
    }
    if (src.holds<inner_ethernet>()) {
        const auto& p_src = src.get<inner_ethernet>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketInnerEthernetProtocolCounters>();

        p_dst->setIp(p_src[inner_ethernet::index::ether]);
        p_dst->setVlan(p_src[inner_ethernet::index::vlan]);
        p_dst->setQinq(p_src[inner_ethernet::index::qinq]);

        dst->setInnerEthernet(p_dst);
    }
    if (src.holds<inner_ip>()) {
        const auto& p_src = src.get<inner_ip>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketInnerIpProtocolCounters>();

        p_dst->setIpv4(p_src[inner_ip::index::ipv4]
                       + p_src[inner_ip::index::ipv4_ext]
                       + p_src[inner_ip::index::ipv4_ext_unknown]);
        p_dst->setIpv6(p_src[inner_ip::index::ipv6]
                       + p_src[inner_ip::index::ipv6_ext]
                       + p_src[inner_ip::index::ipv6_ext_unknown]);

        dst->setInnerIp(p_dst);
    }
    if (src.holds<inner_transport>()) {
        const auto& p_src = src.get<inner_transport>();
        auto p_dst = std::make_shared<
            swagger::v1::model::PacketInnerTransportProtocolCounters>();

        p_dst->setTcp(p_src[inner_transport::index::tcp]);
        p_dst->setUdp(p_src[inner_transport::index::udp]);
        p_dst->setFragmented(p_src[inner_transport::index::fragment]);
        p_dst->setSctp(p_src[inner_transport::index::sctp]);
        p_dst->setIcmp(p_src[inner_transport::index::icmp]);
        p_dst->setNonFragmented(p_src[inner_transport::index::non_fragment]);

        dst->setInnerTransport(p_dst);
    }
}

} // namespace openperf::packet::statistics::api

#endif /* _OP_PACKET_STATISTICS_API_TRANSMOGRIFY_HPP_ */
