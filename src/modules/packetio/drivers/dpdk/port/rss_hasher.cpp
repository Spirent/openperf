#include "packetio/drivers/dpdk/port/rss_hasher.hpp"
#include "utils/prefetch_for_each.hpp"

namespace openperf::packetio::dpdk::port {

constexpr uint32_t decode_mask =
    RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK;

/*
 * Compiler machinations to get the best software RSS hasher
 */

enum class order {
    little = __ORDER_LITTLE_ENDIAN__,
    big = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
};

constexpr auto rss_key_length = 40;
using rss_key_type = std::array<uint8_t, rss_key_length>;

/* Toeplitz hash key */
constexpr auto rss_key = rss_key_type{
    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2, 0x41, 0x67,
    0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0, 0xd0, 0xca, 0x2b, 0xcb,
    0xae, 0x7b, 0x30, 0xb4, 0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30,
    0xf2, 0x0c, 0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa,
};

static rss_key_type get_rss_key()
{
    static_assert(order::native == order::big
                  || order::native == order::little);

    if constexpr (order::native == order::big) {
        auto rss_key_be = rss_key_type{};
        rte_convert_rss_key(reinterpret_cast<const uint32_t*>(rss_key.data()),
                            reinterpret_cast<uint32_t*>(rss_key_be.data()),
                            rss_key.size());
        return (rss_key_be);
    } else {
        return (rss_key);
    }
}

static uint32_t softrss_impl(uint32_t data[], uint32_t length)
{
    static_assert(order::native == order::big
                  || order::native == order::little);

    static const auto key = get_rss_key();

    if constexpr (order::native == order::big) {
        return (rte_softrss_be(data, length, key.data()));
    } else {
        return (rte_softrss(data, length, key.data()));
    }
}

static uint32_t softrss(const rte_thash_tuple& tuple, uint32_t length)
{
    /* XXX: work around DPDK type sloppiness */
    auto* ptr = const_cast<uint32_t*>(
        reinterpret_cast<const uint32_t*>(std::addressof(tuple)));
    return (softrss_impl(ptr, length));
}

static uint32_t calculate_ipv4_hash(const rte_mbuf* mbuf,
                                    const rte_net_hdr_lens& hdr_lens,
                                    uint32_t ptype)
{
    const auto* ipv4 =
        rte_pktmbuf_mtod_offset(mbuf, rte_ipv4_hdr*, hdr_lens.l2_len);
    auto tuple = rte_thash_tuple{.v4 = {.src_addr = ntohl(ipv4->src_addr),
                                        .dst_addr = ntohl(ipv4->dst_addr)}};

    switch (ptype & RTE_PTYPE_L4_MASK) {
    case RTE_PTYPE_L4_UDP: {
        const auto* udp = rte_pktmbuf_mtod_offset(
            mbuf, rte_udp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v4.dport = ntohs(udp->dst_port);
        tuple.v4.sport = ntohs(udp->src_port);
        return (softrss(tuple, RTE_THASH_V4_L4_LEN));
    }
    case RTE_PTYPE_L4_TCP: {
        const auto* tcp = rte_pktmbuf_mtod_offset(
            mbuf, rte_tcp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v4.dport = ntohs(tcp->dst_port);
        tuple.v4.sport = ntohs(tcp->src_port);
        return (softrss(tuple, RTE_THASH_V4_L4_LEN));
    }
    case RTE_PTYPE_L4_SCTP: {
        const auto* sctp = rte_pktmbuf_mtod_offset(
            mbuf, rte_sctp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v4.sctp_tag = ntohl(sctp->tag);
        return (softrss(tuple, RTE_THASH_V4_L4_LEN));
    }
    default:
        return (softrss(tuple, RTE_THASH_V4_L3_LEN));
    }
}

static uint32_t calculate_ipv6_hash(const rte_mbuf* mbuf,
                                    const rte_net_hdr_lens& hdr_lens,
                                    uint32_t ptype)
{
    const auto* ipv6 =
        rte_pktmbuf_mtod_offset(mbuf, rte_ipv6_hdr*, hdr_lens.l2_len);
    auto tuple = rte_thash_tuple{};
    rte_thash_load_v6_addrs(ipv6, std::addressof(tuple));

    switch (ptype & RTE_PTYPE_L4_MASK) {
    case RTE_PTYPE_L4_UDP: {
        const auto* udp = rte_pktmbuf_mtod_offset(
            mbuf, rte_udp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v6.dport = ntohs(udp->dst_port);
        tuple.v6.sport = ntohs(udp->src_port);
        return (softrss(tuple, RTE_THASH_V6_L4_LEN));
    }
    case RTE_PTYPE_L4_TCP: {
        const auto* tcp = rte_pktmbuf_mtod_offset(
            mbuf, rte_tcp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v6.dport = ntohs(tcp->dst_port);
        tuple.v6.sport = ntohs(tcp->src_port);
        return (softrss(tuple, RTE_THASH_V6_L4_LEN));
    }
    case RTE_PTYPE_L4_SCTP: {
        const auto* sctp = rte_pktmbuf_mtod_offset(
            mbuf, rte_sctp_hdr*, hdr_lens.l2_len + hdr_lens.l3_len);
        tuple.v6.sctp_tag = ntohl(sctp->tag);
        return (softrss(tuple, RTE_THASH_V6_L4_LEN));
    }
    default:
        return (softrss(tuple, RTE_THASH_V6_L3_LEN));
    }
}

static uint16_t hash_headers([[maybe_unused]] uint16_t port_id,
                             [[maybe_unused]] uint16_t queue_id,
                             rte_mbuf* packets[],
                             uint16_t nb_packets,
                             [[maybe_unused]] uint16_t max_packets,
                             [[maybe_unused]] void* user_param)
{
    utils::prefetch_for_each(
        packets,
        packets + nb_packets,
        [](const auto* mbuf) { rte_prefetch0(rte_pktmbuf_mtod(mbuf, void*)); },
        [](auto* mbuf) {
            auto hdr_lens = rte_net_hdr_lens{};
            const auto ptype = rte_net_get_ptype(mbuf, &hdr_lens, decode_mask);
            if (RTE_ETH_IS_IPV4_HDR(ptype)) {
                mbuf->hash.rss = calculate_ipv4_hash(mbuf, hdr_lens, ptype);
            } else if (RTE_ETH_IS_IPV6_HDR(ptype)) {
                mbuf->hash.rss = calculate_ipv6_hash(mbuf, hdr_lens, ptype);
            }
        },
        mbuf_prefetch_offset);

    return (nb_packets);
}

std::string callback_rss_hasher::description() { return ("RSS hashing"); }

rx_callback<callback_rss_hasher>::rx_callback_fn callback_rss_hasher::callback()
{
    return (hash_headers);
}

static rss_hasher::variant_type make_rss_hasher(uint16_t port_id)
{
    if (port_info::rss_offloads(port_id) == 0) {
        return (callback_rss_hasher(port_id));
    }

    return (null_feature(port_id));
}

rss_hasher::rss_hasher(uint16_t port_id)
    : feature(make_rss_hasher(port_id))
{}

} // namespace openperf::packetio::dpdk::port
