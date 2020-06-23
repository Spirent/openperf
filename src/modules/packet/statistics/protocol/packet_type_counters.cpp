#include "packet/statistics/generic_protocol_counters.hpp"

namespace openperf::packet::statistics::protocol {

void dump(std::ostream& os, const ethernet& stat)
{
    os << "Ethernet:" << std::endl;

#define WRITE_ETHERNET_VALUE(idx)                                              \
    do {                                                                       \
        os << " " << #idx << ":" << stat[ethernet::index::idx] << std::endl;   \
    } while (0)

    WRITE_ETHERNET_VALUE(ether);
    WRITE_ETHERNET_VALUE(timesync);
    WRITE_ETHERNET_VALUE(arp);
    WRITE_ETHERNET_VALUE(lldp);
    WRITE_ETHERNET_VALUE(nsh);
    WRITE_ETHERNET_VALUE(vlan);
    WRITE_ETHERNET_VALUE(qinq);
    WRITE_ETHERNET_VALUE(pppoe);
    WRITE_ETHERNET_VALUE(fcoe);
    WRITE_ETHERNET_VALUE(mpls);

#undef WRITE_ETHERNET_VALUE
}

void dump(std::ostream& os, const ip& stat)
{
    os << "IP:" << std::endl;

#define WRITE_IP_VALUE(idx)                                                    \
    do {                                                                       \
        os << " " << #idx << ":" << stat[ip::index::idx] << std::endl;         \
    } while (0)

    WRITE_IP_VALUE(ipv4);
    WRITE_IP_VALUE(ipv4_ext);
    WRITE_IP_VALUE(ipv4_ext_unknown);
    WRITE_IP_VALUE(ipv6);
    WRITE_IP_VALUE(ipv6_ext);
    WRITE_IP_VALUE(ipv6_ext_unknown);

#undef WRITE_IP_VALUE
}

void dump(std::ostream& os, const transport& stat)
{
    os << "Transport:" << std::endl;

#define WRITE_TRANSPORT_VALUE(idx)                                             \
    do {                                                                       \
        os << " " << #idx << ":" << stat[transport::index::idx] << std::endl;  \
    } while (0)

    WRITE_TRANSPORT_VALUE(tcp);
    WRITE_TRANSPORT_VALUE(udp);
    WRITE_TRANSPORT_VALUE(fragment);
    WRITE_TRANSPORT_VALUE(sctp);
    WRITE_TRANSPORT_VALUE(icmp);
    WRITE_TRANSPORT_VALUE(non_fragment);
    WRITE_TRANSPORT_VALUE(igmp);

#undef WRITE_TRANSPORT_VALUE
}

void dump(std::ostream& os, const tunnel& stat)
{
    os << "Tunnel:" << std::endl;

#define WRITE_TUNNEL_VALUE(idx)                                                \
    do {                                                                       \
        os << " " << #idx << ":" << stat[tunnel::index::idx] << std::endl;     \
    } while (0)

    WRITE_TUNNEL_VALUE(ip);
    WRITE_TUNNEL_VALUE(gre);
    WRITE_TUNNEL_VALUE(vxlan);
    WRITE_TUNNEL_VALUE(nvgre);
    WRITE_TUNNEL_VALUE(geneve);
    WRITE_TUNNEL_VALUE(grenat);
    WRITE_TUNNEL_VALUE(gtpc);
    WRITE_TUNNEL_VALUE(gtpu);
    WRITE_TUNNEL_VALUE(esp);
    WRITE_TUNNEL_VALUE(l2tp);
    WRITE_TUNNEL_VALUE(vxlan_gpe);
    WRITE_TUNNEL_VALUE(mpls_in_gre);
    WRITE_TUNNEL_VALUE(mpls_in_udp);

#undef WRITE_TUNNEL_VALUE
}

void dump(std::ostream& os, const inner_ethernet& stat)
{
    os << "Inner Ethernet:" << std::endl;

#define WRITE_INNER_ETHERNET_VALUE(idx)                                        \
    do {                                                                       \
        os << " " << #idx << ":" << stat[inner_ethernet::index::idx]           \
           << std::endl;                                                       \
    } while (0)

    WRITE_INNER_ETHERNET_VALUE(ether);
    WRITE_INNER_ETHERNET_VALUE(vlan);
    WRITE_INNER_ETHERNET_VALUE(qinq);

#undef WRITE_INNER_ETHERNET_VALUE
}

void dump(std::ostream& os, const inner_ip& stat)
{
    os << "Inner IP:" << std::endl;

#define WRITE_INNER_IP_VALUE(idx)                                              \
    do {                                                                       \
        os << " " << #idx << ":" << stat[inner_ip::index::idx] << std::endl;   \
    } while (0)

    WRITE_INNER_IP_VALUE(ipv4);
    WRITE_INNER_IP_VALUE(ipv4_ext);
    WRITE_INNER_IP_VALUE(ipv4_ext_unknown);
    WRITE_INNER_IP_VALUE(ipv6);
    WRITE_INNER_IP_VALUE(ipv6_ext);
    WRITE_INNER_IP_VALUE(ipv6_ext_unknown);

#undef WRITE_INNER_IP_VALUE
}

void dump(std::ostream& os, const inner_transport& stat)
{
    os << "Inner Transport:" << std::endl;
#define WRITE_INNER_TRANSPORT_VALUE(idx)                                       \
    do {                                                                       \
        os << " " << #idx << ":" << stat[inner_transport::index::idx]          \
           << std::endl;                                                       \
    } while (0)

    WRITE_INNER_TRANSPORT_VALUE(tcp);
    WRITE_INNER_TRANSPORT_VALUE(udp);
    WRITE_INNER_TRANSPORT_VALUE(fragment);
    WRITE_INNER_TRANSPORT_VALUE(sctp);
    WRITE_INNER_TRANSPORT_VALUE(icmp);
    WRITE_INNER_TRANSPORT_VALUE(non_fragment);

#undef WRITE_INNER_TRANSPORT_VALUE
}

} // namespace openperf::packet::statistics::protocol
