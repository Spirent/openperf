#include <cassert>
#include <variant>

#include "packet/type/net_types.hpp"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/workers/dpdk/worker_api.hpp"
#include "packetio/workers/dpdk/worker_gro.hpp"

namespace openperf::packetio::dpdk::worker {

static constexpr auto max_segment_length = 32767;

/* Enum for specifying relation between packets */
enum class tcp_seq_relation { none = 0, prev, next };

using ip_address =
    std::variant<libpacket::type::ipv4_address, libpacket::type::ipv6_address>;

/*
 * Identify TCP flows via the following key:
 * src/dst ethernet address
 * src/dst ip address
 * src/dst port number
 * ack number
 */
using tcp_flow_key = std::tuple<libpacket::type::mac_address,
                                libpacket::type::mac_address,
                                ip_address,
                                ip_address,
                                uint16_t,
                                uint16_t,
                                uint32_t>;

struct gro_tcp_flow
{
    tcp_flow_key key;
    uint32_t start_index;
};

struct gro_tcp_segment
{
    struct rte_mbuf* first;
    struct rte_mbuf* last;
    std::optional<uint32_t> next_idx;
    uint32_t sent_seq;
    uint16_t nb_mbufs;
    uint16_t ip_id;
    bool is_atomic;
};

/*
 * TCP flows and segments are stored in a fixed size table. So long
 * as the packet burst we process is the same size as the table, we
 * will always have space for every packet we come across, and hence
 * don't have to worry about insertion failures.
 */
template <size_t N> struct gro_tcp_table
{
    std::array<gro_tcp_flow, N> flows;
    std::array<gro_tcp_segment, N> segments;
    size_t nb_flows = 0;
    size_t nb_segments = 0;
};

/***
 * Flow key functions
 ***/
tcp_flow_key get_ipv4_tcp_flow_key(const struct rte_mbuf* packet)
{
    const auto* eth_hdr = rte_pktmbuf_mtod(packet, const struct rte_ether_hdr*);
    const auto* ip4_hdr = rte_pktmbuf_mtod_offset(
        packet, const struct rte_ipv4_hdr*, packet->l2_len);
    const auto* tcp_hdr = rte_pktmbuf_mtod_offset(
        packet, const struct rte_tcp_hdr*, packet->l2_len + packet->l3_len);

    using namespace libpacket::type;

    return (tcp_flow_key{mac_address(eth_hdr->src_addr.addr_bytes),
                         mac_address(eth_hdr->dst_addr.addr_bytes),
                         ipv4_address(ntohl(ip4_hdr->src_addr)),
                         ipv4_address(ntohl(ip4_hdr->dst_addr)),
                         tcp_hdr->src_port,
                         tcp_hdr->dst_port,
                         tcp_hdr->recv_ack});
}

tcp_flow_key get_ipv6_tcp_flow_key(const struct rte_mbuf* packet)
{
    const auto* eth_hdr = rte_pktmbuf_mtod(packet, const struct rte_ether_hdr*);
    const auto* ip6_hdr = rte_pktmbuf_mtod_offset(
        packet, const struct rte_ipv6_hdr*, packet->l2_len);
    const auto* tcp_hdr = rte_pktmbuf_mtod_offset(
        packet, const struct rte_tcp_hdr*, packet->l2_len + packet->l3_len);

    using namespace libpacket::type;

    return (tcp_flow_key{mac_address(eth_hdr->src_addr.addr_bytes),
                         mac_address(eth_hdr->dst_addr.addr_bytes),
                         ipv6_address(ip6_hdr->src_addr),
                         ipv6_address(ip6_hdr->dst_addr),
                         tcp_hdr->src_port,
                         tcp_hdr->dst_port,
                         tcp_hdr->recv_ack});
}

tcp_flow_key get_tcp_flow_key(const struct rte_mbuf* packet)
{
    return (RTE_ETH_IS_IPV4_HDR(packet->packet_type)
                ? get_ipv4_tcp_flow_key(packet)
                : get_ipv6_tcp_flow_key(packet));
}

/***
 * Turn a segment into a packet
 ***/
static struct rte_mbuf* get_ipv4_output_packet(struct rte_mbuf* packet)
{
    auto* ip4_hdr =
        rte_pktmbuf_mtod_offset(packet, struct rte_ipv4_hdr*, packet->l2_len);
    ip4_hdr->total_length = htons(rte_pktmbuf_pkt_len(packet) - packet->l2_len);
    return (packet);
}

static struct rte_mbuf* get_ipv6_output_packet(struct rte_mbuf* packet)
{
    auto* ip6_hdr =
        rte_pktmbuf_mtod_offset(packet, struct rte_ipv6_hdr*, packet->l2_len);
    ip6_hdr->payload_len = htons(rte_pktmbuf_pkt_len(packet) - packet->l2_len
                                 - sizeof(struct rte_ipv6_hdr));
    return (packet);
}

static struct rte_mbuf* get_output_packet(struct gro_tcp_segment& segment)
{
    if (segment.nb_mbufs == 1) { return (segment.first); }

    auto* head = segment.first;

    /* Update TCP header */
    auto* tcp_hdr = rte_pktmbuf_mtod_offset(
        head, struct rte_tcp_hdr*, head->l2_len + head->l3_len);

    /* Make sure sequence number is correct */
    if (segment.sent_seq != ntohl(tcp_hdr->sent_seq)) {
        tcp_hdr->sent_seq = htonl(segment.sent_seq);
    }

    return (RTE_ETH_IS_IPV4_HDR(head->packet_type)
                ? get_ipv4_output_packet(head)
                : get_ipv6_output_packet(head));
}

/***
 * Helpful debugging functions
 ***/
void dump(const struct gro_tcp_segment& seg)
{
    fprintf(stderr,
            " seg: first = %p, last = %p, next_idx = %d, sent_seq = %u, "
            "nb_mbufs = %u (length = %u), ip_id = %u, is_atomic = %s\n",
            (void*)seg.first,
            (void*)seg.last,
            seg.next_idx.value_or(-1),
            seg.sent_seq,
            seg.nb_mbufs,
            rte_pktmbuf_pkt_len(seg.first),
            seg.ip_id,
            seg.is_atomic ? "true" : "false");
}

static std::string to_string(const ip_address& ip)
{
    return (std::visit(
        [](const auto& addr) { return (libpacket::type::to_string(addr)); },
        ip));
}

void dump(const tcp_flow_key& key)
{
    using namespace libpacket::type;

    fprintf(stderr,
            " flow: %s/%s/%s/%s/%u/%u/%u\n",
            to_string(std::get<0>(key)).c_str(),
            to_string(std::get<1>(key)).c_str(),
            to_string(std::get<2>(key)).c_str(),
            to_string(std::get<3>(key)).c_str(),
            ntohs(std::get<4>(key)),
            ntohs(std::get<5>(key)),
            ntohl(std::get<6>(key)));
}

template <size_t N> void dump(const struct gro_tcp_table<N>& table)
{
    fprintf(stderr, "table\n");
    for (auto i = 0U; i < table.nb_flows; i++) { dump(table.flows[i].key); }
    for (auto i = 0U; i < table.nb_segments; i++) { dump(table.segments[i]); }
}

/***
 * Table manipulation functions
 ***/
template <size_t N>
static size_t insert_tcp_flow(struct gro_tcp_table<N>& table,
                              tcp_flow_key&& key,
                              size_t item_idx)
{
    assert(table.nb_flows != N);

    auto flow_idx = table.nb_flows;
    auto& flow = table.flows[flow_idx];
    flow.key = key;
    flow.start_index = item_idx;
    table.nb_flows++;

    return (flow_idx);
}

template <size_t N>
static size_t insert_tcp_packet(struct gro_tcp_table<N>& table,
                                struct rte_mbuf* packet,
                                std::optional<size_t> prev_idx,
                                uint32_t sent_seq,
                                uint16_t ip_id,
                                bool is_atomic)
{
    assert(table.nb_segments != N);

    auto seg_idx = table.nb_segments;
    auto& seg = table.segments[seg_idx];
    seg.first = packet;
    seg.last = rte_pktmbuf_lastseg(packet);
    seg.next_idx = std::nullopt;
    seg.sent_seq = sent_seq;
    seg.nb_mbufs = 1;
    seg.ip_id = ip_id;
    seg.is_atomic = is_atomic;
    table.nb_segments++;

    if (prev_idx) {
        seg.next_idx = table.segments[*prev_idx].next_idx;
        table.segments[*prev_idx].next_idx = seg_idx;
    }

    return (seg_idx);
}

static int merge_tcp_packet(struct gro_tcp_segment& segment,
                            struct rte_mbuf* pkt,
                            enum tcp_seq_relation cmp,
                            uint32_t tcp_seq_number,
                            uint16_t ip_id)
{
    assert(cmp != tcp_seq_relation::none);

    struct rte_mbuf *pkt_head = nullptr, *pkt_tail = nullptr;

    if (cmp == tcp_seq_relation::next) {
        pkt_head = segment.first;
        pkt_tail = pkt;
    } else {
        pkt_head = pkt;
        pkt_tail = segment.first;
    }

    auto tail_hdr_len = pkt_tail->l2_len + pkt_tail->l3_len + pkt_tail->l4_len;
    if (rte_pktmbuf_pkt_len(pkt_head) - pkt_head->l2_len
            + rte_pktmbuf_pkt_len(pkt_tail) - tail_hdr_len
        > max_segment_length) {
        return (-1); /* too big to merge */
    }

    /* Drop header from tail packet */
    rte_pktmbuf_adj(pkt_tail, tail_hdr_len);

    if (cmp == tcp_seq_relation::next) {
        segment.last->next = pkt_tail;
        segment.last = rte_pktmbuf_lastseg(pkt_tail);
    } else {
        rte_pktmbuf_lastseg(pkt_head)->next = segment.first;
        segment.first = pkt_head;
        segment.sent_seq = tcp_seq_number;
    }
    segment.ip_id = ip_id;
    segment.nb_mbufs++;

    pkt_head->nb_segs += pkt_tail->nb_segs;
    pkt_head->pkt_len += pkt_tail->pkt_len;

    return (0);
}

/***
 * Utility functions
 ***/
static std::pair<uint16_t, bool>
get_fragment_info(const struct rte_mbuf* packet)
{
    /* No fragment info for IPv6 */
    if (RTE_ETH_IS_IPV6_HDR(packet->packet_type)) { return {0, true}; }

    const auto* ip4_hdr = rte_pktmbuf_mtod_offset(
        packet, const struct rte_ipv4_hdr*, packet->l2_len);
    auto frag_off = htons(ip4_hdr->fragment_offset);
    auto is_atomic = (frag_off & RTE_IPV4_HDR_DF_FLAG) == RTE_IPV4_HDR_DF_FLAG;
    auto ip_id = is_atomic ? 0 : htons(ip4_hdr->packet_id);
    return {ip_id, is_atomic};
}

static bool is_tcp_packet(const struct rte_mbuf* m)
{
    return ((RTE_ETH_IS_IPV4_HDR(m->packet_type)
             || RTE_ETH_IS_IPV6_HDR(m->packet_type))
            && ((m->packet_type & RTE_PTYPE_L4_TCP) == RTE_PTYPE_L4_TCP)
            && (RTE_ETH_IS_TUNNEL_PKT(m->packet_type) == 0));
}

static bool is_invalid_tcp_length(size_t length)
{
    return (length < sizeof(rte_tcp_hdr) || length > 60);
}

static bool is_ipv6_packet_with_options(const struct rte_mbuf* packet)
{
    if (RTE_ETH_IS_IPV6_HDR(packet->packet_type)
        && packet->l3_len > sizeof(struct rte_ipv6_hdr)) {
        return (true);
    }

    return (false);
}

/*
 * Compare the sequence number in the given header with the data
 * in our segment
 */
enum tcp_seq_relation get_tcp_sequence_relation(struct gro_tcp_segment& segment,
                                                struct rte_tcp_hdr* tcp_header,
                                                uint32_t tcp_seq_number,
                                                uint16_t tcp_header_len,
                                                uint16_t tcp_data_len,
                                                uint16_t ip_id,
                                                bool is_atomic)
{
    auto* seg_hdr =
        rte_pktmbuf_mtod_offset(segment.first,
                                struct rte_tcp_hdr*,
                                segment.first->l2_len + segment.first->l3_len);
    uint16_t seg_hdr_len = segment.first->l4_len;

    /* Verify TCP options are equal */
    auto opt_len =
        std::max(tcp_header_len, seg_hdr_len) - sizeof(struct rte_tcp_hdr);
    if ((tcp_header_len != seg_hdr_len)
        || ((opt_len > 0)
            && (memcmp(tcp_header + 1, seg_hdr + 1, opt_len) != 0))) {
        return (tcp_seq_relation::none);
    }

    /* Verify DF bits are equal */
    if (segment.is_atomic != is_atomic) { return (tcp_seq_relation::none); }

    /* Verify packets are adjacent */
    auto seg_len = rte_pktmbuf_pkt_len(segment.first) - segment.first->l2_len
                   - segment.first->l3_len - seg_hdr_len;
    if ((tcp_seq_number == segment.sent_seq + seg_len)
        && (is_atomic || (ip_id == segment.ip_id + 1))) {
        return (tcp_seq_relation::next);
    } else if ((tcp_seq_number + tcp_data_len == segment.sent_seq)
               && (is_atomic || (ip_id + segment.nb_mbufs == segment.ip_id))) {
        return (tcp_seq_relation::prev);
    }

    /* No relation */
    return (tcp_seq_relation::none);
}

/***
 * Reassembly functions
 ***/
template <size_t N>
int do_tcp_reassemble(struct gro_tcp_table<N>& table, struct rte_mbuf* packet)
{
    /* Sanity check */
    if (is_invalid_tcp_length(packet->l4_len)) { return (-1); }

    /*
     * Unsafe to reassemble IPv6 packets with options, since those
     * options could contain fragments.
     */
    if (is_ipv6_packet_with_options(packet)) { return (-1); }

    /* Don't merge TCP packets with any non-bulk transfer flags set. */
    auto* tcp_hdr = rte_pktmbuf_mtod_offset(
        packet, struct rte_tcp_hdr*, packet->l2_len + packet->l3_len);
    if (tcp_hdr->tcp_flags & ~(RTE_TCP_ACK_FLAG | RTE_TCP_PSH_FLAG)) {
        return (-1);
    }

    /* No payload -> nothing to merge */
    auto hdr_len = packet->l2_len + packet->l3_len + packet->l4_len;
    auto tcp_payload_len = rte_pktmbuf_pkt_len(packet) - hdr_len;
    if (tcp_payload_len <= 0) { return (-1); }

    /* At this point, the packet is something we can safely merge. */
    auto [ip_id, is_atomic] = get_fragment_info(packet);
    auto sent_seq = ntohl(tcp_hdr->sent_seq);
    auto key = get_tcp_flow_key(packet);
    auto flow_end = table.flows.data() + table.nb_flows;

    /* Have we seen this flow before? */
    auto flow =
        std::find_if(table.flows.data(), flow_end, [&](const auto& flow) {
            return (key == flow.key);
        });
    if (flow == flow_end) {
        /* Nope; insert both flow and packet into our table */
        insert_tcp_flow(
            table,
            std::move(key),
            insert_tcp_packet(
                table, packet, std::nullopt, sent_seq, ip_id, is_atomic));
        return (0);
    }

    /*
     * Packet matches an existing flow. Look for adjacent packets and merge
     * them together. If no adjacency is found, just store the packet as a
     * separate segment.
     */
    std::optional<uint32_t> cur_idx = flow->start_index;
    auto prev_idx = cur_idx;
    do {
        auto cmp = get_tcp_sequence_relation(table.segments[cur_idx.value()],
                                             tcp_hdr,
                                             sent_seq,
                                             packet->l4_len,
                                             tcp_payload_len,
                                             ip_id,
                                             is_atomic);
        if (cmp != tcp_seq_relation::none) {
            if (merge_tcp_packet(table.segments[cur_idx.value()],
                                 packet,
                                 cmp,
                                 sent_seq,
                                 ip_id)
                != 0) {
                /* Merge failed; insert packet as new segment */
                insert_tcp_packet(
                    table, packet, prev_idx, sent_seq, ip_id, is_atomic);
            }
            return (0);
        }

        prev_idx = cur_idx;
        cur_idx = table.segments[cur_idx.value()].next_idx;
    } while (cur_idx);

    /* No neighbor; insert packet as a new segment */
    insert_tcp_packet(table, packet, prev_idx, sent_seq, ip_id, is_atomic);

    return (0);
}

static uint16_t do_gro_reassemble(struct rte_mbuf* packets[], uint16_t n)
{
    assert(n <= pkt_burst_size);

    auto tcp_table = gro_tcp_table<pkt_burst_size>{};
    auto skipped_packets = std::array<struct rte_mbuf*, pkt_burst_size>{};
    auto nb_skips = 0U;

    /*
     * Attempt to reassemble TCP packets or store unprocessed
     * packets in the skipped_packets array.
     */
    std::for_each(packets, packets + n, [&](auto* m) {
        if (!is_tcp_packet(m) || do_tcp_reassemble(tcp_table, m) != 0) {
            skipped_packets[nb_skips++] = m;
        }
    });

    /* Using packets as our output array, write tcp segments to packets */
    std::transform(tcp_table.segments.data(),
                   tcp_table.segments.data() + tcp_table.nb_segments,
                   packets,
                   get_output_packet);

    /* Copy skipped data to packets */
    std::copy(skipped_packets.data(),
              skipped_packets.data() + nb_skips,
              &packets[tcp_table.nb_segments]);

    /* Return the new number of pointers in packets */
    return (tcp_table.nb_segments + nb_skips);
}

uint16_t do_software_gro(struct rte_mbuf* packets[], uint16_t n)
{
    /* Skip performing GRO if there is no potential benefit */
    if (n == 1 || std::count_if(packets, packets + n, is_tcp_packet) < 2) {
        return (n);
    }

    /*
     * We need to be able to inspect header data in order to do
     * reassembly. Hence, parse the headers and store the header
     * offsets in the packet metadata.
     */
    std::for_each(packets, packets + n, [](auto* m) {
        if (is_tcp_packet(m)) {
            struct rte_net_hdr_lens hdr_lens = {};
            rte_net_get_ptype(
                m,
                &hdr_lens,
                (RTE_PTYPE_L2_MASK | RTE_PTYPE_L3_MASK | RTE_PTYPE_L4_MASK));
            m->l2_len = hdr_lens.l2_len;
            m->l3_len = hdr_lens.l3_len;
            m->l4_len = hdr_lens.l4_len;
        }
    });

    /* Finally, try to do some reassembly */
    return (do_gro_reassemble(packets, n));
}

} // namespace openperf::packetio::dpdk::worker
