/**
 * @file
 *
 * The content of some of these functions are based on the lwip/GSO implementation contained
 * here: https://github.com/sysml/lwip.git
 */

#include <algorithm>
#include <cassert>

#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"
#include "packetio/drivers/dpdk/model/port_info.h"
#include "packetio/memory/dpdk/pbuf_utils.h"
#include "packetio/stack/dpdk/net_interface.h"
#include "packetio/stack/gso_utils.h"

namespace icp {
namespace packetio {
namespace dpdk {
namespace gso {

/*
 * Nearly identical to the pbuf_skip function in <lwip>/src/core/pbuf.c,
 * but also returns the pbuf before the returned pbuf.
 */
static
pbuf* pbuf_skip_with_previous(struct pbuf *in, u16_t in_offset,
                              u16_t *out_offset, struct pbuf** out_prev)
{
    u16_t offset_left = in_offset;
    struct pbuf *q = in;
    struct pbuf *prev = NULL;

    /* get the correct pbuf */
    while ((q != NULL) && (q->len <= offset_left)) {
        offset_left = (u16_t)(offset_left - q->len);
        prev = q;
        q = q->next;
    }
    if (out_offset) *out_offset = offset_left;
    if (out_prev)   *out_prev = prev;

    return q;
}

/* Drop length bytes from the given pbuf chain at the specified offset. */
static
std::tuple<pbuf*, uint16_t> pbuf_drop_at(struct pbuf* p_head, uint16_t offset, uint16_t length)
{
    LWIP_ERROR("(p_head != NULL) && (offset + len < p_head->tot_len) (programmer violates API)",
               ((p_head != NULL) && (offset + length < p_head->tot_len)),
               return (std::make_tuple(nullptr, 0)););

    for (struct pbuf* p = p_head; p != NULL; p = p->next)
        LWIP_ASSERT("pbuf chains with multiple references are not implemented yet", (p->ref <= 1));

    if (offset == 0 && length == p_head->tot_len) {
        /* easy case; drop the whole chain */
        auto clen = pbuf_clen(p_head);
        pbuf_free(p_head);
        return (std::make_tuple(nullptr, clen));
    }

    uint16_t start_offset = 0;
    struct pbuf* start_prev = NULL;
    auto start = pbuf_skip_with_previous(p_head, offset, &start_offset, &start_prev);
    if (start_offset == 0 && start_prev != NULL) {
        start = start_prev;
        start_offset = start_prev->len;
    }
    LWIP_ASSERT("start != NULL", (start != NULL));

    uint16_t end_offset = 0;
    auto end = pbuf_skip(p_head, offset + length, &end_offset);

    uint16_t pbufs_freed = 0;
    auto flags = p_head->flags; /* we'll need these later */
    if (start == end) {
        /* Drop bytes by shuffling the data around */
        memmove(reinterpret_cast<uint8_t*>(start->payload) + length,
                reinterpret_cast<uint8_t*>(start->payload),
                start_offset);
        start->payload = reinterpret_cast<uint8_t*>(start->payload) + length;
        start->len -= length;
        start->tot_len -= length;
    } else {
        /* Drop bytes on leading pbuf? */
        if (offset != 0 && start_offset < start->len) {
            start->len = start_offset;
        }

        /* Drop bytes on trailing pbuf? */
        if (end != NULL && end_offset > 0) {
            end->payload = reinterpret_cast<uint8_t*>(end->payload) + end_offset;
            end->len -= end_offset;
            end->tot_len -= end_offset;
        }

        /* Drop the stuff in between */
        auto p = start_offset == 0 ? start : start->next;
        while (p != end) {
            auto next = p->next;
            p->next = NULL;
            pbuf_free(p);
            pbufs_freed++;
            p = next;
        }

        /* Update head, if necessary */
        if (offset == 0) p_head = end;
    }

    /* fix up the chain length */
    if (start != end) {
        start->next = end;
        start->tot_len -= length;
    }
    for (auto p = p_head; p != start; p = p->next) {
        p->tot_len -= length;
    }

    p_head->flags = flags;

    return (std::make_tuple(p_head, pbufs_freed));
}

/*
 * Split the specified pbuf chain at the given offset.
 * Returns the a pbuf that contains the chain after the split value
 * and whether a new pbuf was allocated or not.
 * The returned pbuf is guaranteed to have enough headroom for the requested
 * layer.
 */
static
std::tuple<pbuf*, bool> pbuf_split_at(struct pbuf* p_head, uint16_t split, pbuf_layer layer)
{
    LWIP_ERROR("(p_head != NULL) && (split < p_head->tot_len) (programmer violates API)",
               ((p_head != NULL) && (split < p_head->tot_len)),
               return (std::make_tuple(nullptr, false)););

    for (struct pbuf* p = p_head; p != NULL; p = p->next)
        LWIP_ASSERT("pbuf chains with multiple references are not implemented yet", (p->ref <= 1));

    if (split == 0 || split >= p_head->tot_len) {
        /* easy case; nothing to split */
        return (std::make_tuple(nullptr, false));
    }

    pbuf *to_return = nullptr;
    bool new_pbuf = false;
    uint16_t split_offset = 0;
    struct pbuf* split_prev = nullptr;
    struct pbuf* split_pbuf = pbuf_skip_with_previous(p_head, split, &split_offset, &split_prev);

    LWIP_ASSERT("split_pbuf == nullptr", split_pbuf != nullptr);
    LWIP_ASSERT("offset != 0 or prev != 0)", split_offset != 0 || split_prev != 0);

    if (split_offset == 0) {
        /* Nice!  We split on a pbuf boundary */
        LWIP_ASSERT("split pbuf header too small",
                    layer <= packetio_memory_pbuf_header_available(split_pbuf));

        split_prev->next = nullptr;
        for (auto p = p_head; p != nullptr; p = p->next) {
            p->tot_len -= split_pbuf->tot_len;
        }
        to_return = split_pbuf;
    } else {
        /* Not nice; Allocate a new pbuf and update lengths and payload */
        LWIP_ASSERT("supported pbuf type",
                    pbuf_match_type(split_pbuf, PBUF_RAM) || pbuf_match_type(split_pbuf, PBUF_POOL));
        auto next_pbuf = pbuf_alloc(layer, split_pbuf->len - split_offset,
                                    (pbuf_match_type(split_pbuf, PBUF_RAM)
                                     ? PBUF_RAM
                                     : PBUF_POOL));
        if (!next_pbuf) return (std::make_tuple(nullptr, false));

        LWIP_ASSERT("split pbuf header too small",
                    layer <= packetio_memory_pbuf_header_available(next_pbuf));

        next_pbuf->flags = split_pbuf->flags;
        next_pbuf->ref = split_pbuf->ref;

        /* move payload */
        next_pbuf->len = split_pbuf->len - split_offset;
        next_pbuf->tot_len = next_pbuf->len;
        MEMCPY(next_pbuf->payload,
               reinterpret_cast<uint8_t*>(split_pbuf->payload) + split_offset,
               next_pbuf->len);

        split_pbuf->len = split_offset;

        /* relink chains */
        next_pbuf->next = split_pbuf->next;
        split_pbuf->next = nullptr;

        if (next_pbuf->next) next_pbuf->tot_len += next_pbuf->next->tot_len;
        for (auto p = p_head; p != nullptr; p = p->next) {
            p->tot_len -= next_pbuf->tot_len;
        }

        to_return = next_pbuf;
        new_pbuf = true;
    }

    to_return->flags = p_head->flags;

    return (std::make_tuple(to_return, new_pbuf));
}

}
}
}
}

extern "C" {

using namespace icp::packetio::dpdk;

uint16_t packetio_stack_gso_oversize_calc_length(uint16_t length)
{
    /*
     * Verify that payload size is indeed a power of two, otherwise we
     * need to update this calculation.
     */
    static constexpr uint16_t payload_size = PBUF_POOL_BUFSIZE - RTE_PKTMBUF_HEADROOM;
    static_assert(payload_size != 0 && (payload_size & (payload_size - 1)) == 0);
    return ((length + payload_size - 1) & ~(payload_size - 1));
}

uint16_t packetio_stack_gso_max_segment_length(const struct tcp_pcb* pcb)
{
    /*
     * We need to query the outgoing interface for its maximum GSO
     * segment size. If the socket is bound to the interface, then we
     * can look up the interface directly; otherwise we have to do a
     * route lookup.
     *
     * Note: even though the hardware might support larger segments,
     * we must keep the value well below lwIP's maximum of 64k in order
     * to prevent overflow in the various transmit functions.
     */
    auto netif = (pcb->netif_idx != NETIF_NO_INDEX
                  ? netif_get_by_index(pcb->netif_idx)
                  : ip_route(&pcb->local_ip, &pcb->remote_ip));
    if (!netif) return (TCP_MSS);

    auto ifp = reinterpret_cast<net_interface*>(netif->state);
    return (std::max(60U * 1024, ifp->max_gso_length()));
}

uint16_t
packetio_stack_gso_segment_ack_partial(struct tcp_seg *seg, uint16_t acked)
{
    LWIP_ERROR("(seg != NULL) && (acked < seg->len) (programmer violates API)",
               ((seg != NULL) && (acked < seg->len)), return 0;);

    LWIP_DEBUGF(TCP_INPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                ("gso_segment_ack_partial: acked %" PRIu16 " bytes (seg->len: %" PRIu16
                 ", seg->p->tot_len: %" PRIu16 ", hdr_len: %" PRIu32 ")\n",
                 acked, seg->len, seg->p->tot_len, seg->p->tot_len - seg->len));

    LWIP_ASSERT("segment without TCP header!?",
                (seg->tcphdr != NULL && seg->len < seg->p->tot_len));

    /* Easy case; no dropping required */
    if (acked == 0) return 0;

    /* Caclulate drop range */
    auto payload_offset = seg->p->tot_len - seg->len;

    /* Drop bytes from pbuf */
    auto [q, nb_pbufs_freed] = gso::pbuf_drop_at(seg->p, payload_offset, acked);
    LWIP_ASSERT("failed to drop bytes from pbuf", (q != NULL));
    LWIP_ASSERT("dropped/modified segment header!?", (q == seg->p));

    /* Update segment descriptor */
    seg->len -= acked;

    /* Update TCP header fields */
    seg->tcphdr->seqno = lwip_htonl(lwip_ntohl(seg->tcphdr->seqno) + acked);
    if (TCPH_FLAGS(seg->tcphdr) & TCP_URG) {
        auto urgp = lwip_ntohs(seg->tcphdr->urgp);
        if (urgp <= acked) {
            TCPH_UNSET_FLAG(seg->tcphdr, TCP_URG);
            seg->tcphdr->urgp = 0;
        } else {
            seg->tcphdr->urgp = lwip_htons(urgp - acked);
        }
    }

    return (nb_pbufs_freed);
}

int packetio_stack_gso_segment_split(struct tcp_pcb *pcb, struct tcp_seg* seg,
                                     uint16_t split)
{
    LWIP_ERROR("pcb != NULL && seg != NULL && split != 0 && split < seg->len (programmer violates API)",
               ((pcb != nullptr) && (seg != nullptr) && (split != 0) && (split < seg->len)), return ERR_ARG;);

    LWIP_ASSERT("segment without TCP header", (seg->tcphdr != nullptr));

    LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                ("gso_segment_split: split segment at %" PRIu16 " (seg->len: %" PRIu16
                 ", seg->p->tot_len: %" PRIu16 ", hdr_len: %" PRIu16 ")\n",
                 split, seg->len, seg->p->tot_len, seg->p->tot_len - seg->len));

    uint16_t seg_header_length = seg->p->tot_len - seg->len;
    uint16_t tcp_header_length = TCP_HLEN + LWIP_TCP_OPT_LENGTH(seg->flags);

    LWIP_ASSERT("zero length header?", seg_header_length);

    /* Allocate our new segment */
    auto next_seg = reinterpret_cast<tcp_seg*>(memp_malloc(MEMP_TCP_SEG));
    if (!next_seg) {
        LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("gso_segment_split: failed to allocate segment\n"));
        return ERR_MEM;
    }

    /*
     * Split the pbufs into two chains. One remains attached to the segment, the other is
     * returned by this function.
     */
    auto [next_pbuf, pbuf_incr] = gso::pbuf_split_at(seg->p, seg_header_length + split, PBUF_TRANSPORT);
    if (!next_pbuf) {
        LWIP_DEBUGF(TCP_OUTPUT_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("gso_segment_split: failed to split pbuf\n"));
        memp_free(MEMP_TCP_SEG, next_seg);
        return ERR_MEM;
    }

    /* Copy the original TCP header to the new chain */
    auto err = pbuf_header(next_pbuf, tcp_header_length);
    LWIP_ASSERT("appending header failed", err == ERR_OK);  /* can never fail... */
    packetio_stack_gso_pbuf_copy(next_pbuf, 0, seg->tcphdr, tcp_header_length);

    /* Update segments with new chain/length data */
    next_seg->p = next_pbuf;
    next_seg->tcphdr = reinterpret_cast<tcp_hdr*>(next_seg->p->payload);

    next_seg->len = seg->len - split;
    next_seg->flags = seg->flags;
    next_seg->next = seg->next;

    seg->len  = split;
    seg->next = next_seg;

#if TCP_OVERSIZE
    /* Trimming the chain removes any oversize from the original chain */
    seg->oversize_left = 0;

    /*
     * However, if our new segment is the last unsent segment, we probably have some
     * oversize; update the segment and the pcb.
     */
    if (next_seg->next == NULL) {
        /* skip to last pbuf in chain */
        auto p = next_seg->p;
        while (p->next != nullptr) p = p->next;
        LWIP_ASSERT("found last p", p->next == nullptr);
        next_seg->oversize_left = LWIP_MIN(packetio_stack_gso_pbuf_data_available(p, p->len),
                                           TCP_MSS);
        pcb->unsent_oversize = next_seg->oversize_left;
    }
#endif /* TCP_OVERSIZE */

    /* Update TCP header flags on both segments */
    TCPH_UNSET_FLAG(seg->tcphdr, TCP_FIN);
    TCPH_UNSET_FLAG(seg->tcphdr, TCP_PSH);
    TCPH_UNSET_FLAG(seg->tcphdr, TCP_CWR);
    if (TCPH_FLAGS(seg->tcphdr) & TCP_URG) {
        auto urgp = lwip_ntohs(seg->tcphdr->urgp);
        if (urgp <= seg->len) {
            TCPH_UNSET_FLAG(next_seg->tcphdr, TCP_URG);
            next_seg->tcphdr->urgp = 0;
        } else {
            next_seg->tcphdr->urgp = lwip_htons(urgp - seg->len);
            seg->tcphdr->urgp = lwip_htons(seg->len);
        }
    }
    next_seg->tcphdr->seqno = lwip_htonl(lwip_ntohl(seg->tcphdr->seqno) + seg->len);

    /*
     * If we allocated a new pbuf to split the chain, we need to update the
     * queue length.
     */
    if (pbuf_incr) pcb->snd_queuelen++;

    return ERR_OK;
}

uint16_t packetio_stack_gso_pbuf_data_available(const struct pbuf* p, uint16_t offset)
{
    auto max_payload = packetio_memory_pbuf_data_available(p);
    return (max_payload > offset
            ? max_payload - offset
            : 0);
}

uint16_t packetio_stack_gso_pbuf_copy(struct pbuf* p_head, uint16_t offset,
                                      const void* dataptr, uint16_t length)
{
    uint16_t copied = 0;

    for (auto p = p_head; length != 0 && p != nullptr; p = p->next) {
        auto available = packetio_stack_gso_pbuf_data_available(p, offset);
        if (!available) {
            offset -= p->len;
        } else {
            auto to_copy = std::min(available, length);
            MEMCPY(reinterpret_cast<uint8_t*>(p->payload) + offset,
                   reinterpret_cast<const uint8_t*>(dataptr) + copied,
                   to_copy);
            copied += to_copy;
            length -= to_copy;

            offset = 0;
        }
    }

    return (copied);
}

}
