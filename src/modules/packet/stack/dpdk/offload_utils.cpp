#include "rte_ip.h"  /* ip pseudo header cksum */
#include "rte_net.h" /* ptype info */

#include "packet/stack/dpdk/offload_utils.hpp"
#include "core/op_log.h"

namespace openperf::packet::stack::dpdk {

static uint16_t tcp_header_length(const rte_tcp_hdr* tcp)
{
    return ((tcp->data_off & 0xf0) >> 2);
}

/*
 * This is really the brute force approach to checksum offloading.
 * Unfortunately, lwIP doesn't have good hooks to apply checksum
 * data to the packet as it passes through the stack, so we have to
 * do all the work in one swell foop.
 */
void set_tx_offload_metadata(rte_mbuf* mbuf, uint16_t mtu)
{
    /* Parse the packet headers to determine protocols and header offsets */
    struct rte_net_hdr_lens hdr_lens = {};
    auto ptype = rte_net_get_ptype(
        mbuf,
        &hdr_lens,
        (RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK));

    /*
     * Use the packet type data to generate the proper offload flags and to
     * fix up packet headers as necessary, e.g. the hardware needs the
     * pseudo-header checksum for L4 packet types.
     */
    uint64_t ol_flags = 0;
    uint16_t tso_segsz = 0;
    switch (ptype & RTE_PTYPE_L3_MASK) {
    case RTE_PTYPE_L3_IPV4:
    case RTE_PTYPE_L3_IPV4_EXT:
    case RTE_PTYPE_L3_IPV4_EXT_UNKNOWN: {
        auto ip4 =
            rte_pktmbuf_mtod_offset(mbuf, rte_ipv4_hdr*, hdr_lens.l2_len);
        ip4->hdr_checksum = 0;
        ol_flags |= (PKT_TX_IP_CKSUM | PKT_TX_IPV4);
        break;
    }
    case RTE_PTYPE_L3_IPV6:
    case RTE_PTYPE_L3_IPV6_EXT:
    case RTE_PTYPE_L3_IPV6_EXT_UNKNOWN:
        ol_flags |= PKT_TX_IPV6;
        break;
    }

    switch (ptype & RTE_PTYPE_L4_MASK) {
    case RTE_PTYPE_L4_UDP: {
        auto ip = rte_pktmbuf_mtod_offset(mbuf, void*, hdr_lens.l2_len);
        auto udp = rte_pktmbuf_mtod_offset(
            mbuf, rte_udp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        ol_flags |= PKT_TX_UDP_CKSUM;
        udp->dgram_cksum =
            (ol_flags & PKT_TX_IPV4
                 ? rte_ipv4_phdr_cksum(reinterpret_cast<rte_ipv4_hdr*>(ip),
                                       ol_flags)
                 : rte_ipv6_phdr_cksum(reinterpret_cast<rte_ipv6_hdr*>(ip),
                                       ol_flags));
        break;
    }
    case RTE_PTYPE_L4_TCP: {
        auto ip = rte_pktmbuf_mtod_offset(mbuf, void*, hdr_lens.l2_len);
        auto tcp = rte_pktmbuf_mtod_offset(
            mbuf, rte_tcp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tso_segsz = mtu - hdr_lens.l3_len - tcp_header_length(tcp);
        ol_flags |=
            (rte_pktmbuf_pkt_len(mbuf) > mtu ? PKT_TX_TCP_SEG | PKT_TX_TCP_CKSUM
                                             : PKT_TX_TCP_CKSUM);
        tcp->cksum = (ol_flags & PKT_TX_IPV4
                          ? rte_ipv4_phdr_cksum(
                              reinterpret_cast<rte_ipv4_hdr*>(ip), ol_flags)
                          : rte_ipv6_phdr_cksum(
                              reinterpret_cast<rte_ipv6_hdr*>(ip), ol_flags));
        break;
    }
    }

    /* Finally, update mbuf metadata */
    mbuf->ol_flags = ol_flags;
    mbuf->l2_len = hdr_lens.l2_len;
    mbuf->l3_len = hdr_lens.l3_len;
    mbuf->l4_len = hdr_lens.l4_len;
    mbuf->tso_segsz = (ol_flags & PKT_TX_TCP_SEG ? tso_segsz : 0);

    rte_mbuf_sanity_check(mbuf, true);
}

} // namespace openperf::packet::stack::dpdk
