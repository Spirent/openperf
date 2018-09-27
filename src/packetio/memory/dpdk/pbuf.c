/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include <stdatomic.h>
#include <stdbool.h>

#include <sys/types.h>

#include "lwip/opt.h"
#include "lwip/stats.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"

#include "drivers/dpdk/dpdk.h"
#include "memory/dpdk/memp.h"
#include "memory/dpdk/pbuf.h"

extern struct pbuf *packetio_pbuf_synchronize(struct rte_mbuf *m_head);

static uint16_t _get_header_offset(pbuf_layer layer)
{
    uint16_t offset = -1;
    switch (layer) {
    case PBUF_TRANSPORT:
        /* add room for transport (often TCP) layer header */
        offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN + PBUF_TRANSPORT_HLEN;
        break;
    case PBUF_IP:
        /* add room for IP layer header */
        offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN + PBUF_IP_HLEN;
        break;
    case PBUF_LINK:
        /* add room for link layer header */
        offset = PBUF_LINK_ENCAPSULATION_HLEN + PBUF_LINK_HLEN;
        break;
    case PBUF_RAW_TX:
        /* add room for encapsulating link layer headers (e.g. 802.11) */
        offset = PBUF_LINK_ENCAPSULATION_HLEN;
        break;
    case PBUF_RAW:
        /* no offset (e.g. RX buffers or chain successors) */
        offset = 0;
        break;
    default:
        LWIP_ASSERT("pbuf_alloc: bad pbuf layer", 0);
    }

    return (offset);
}

/*
 * We place pbuf data in the mbuf private area.  This allows us to use
 * DPDK mbuf's natively.
 */
struct pbuf *_allocate_dpdk_backed_pbuf(pbuf_layer layer, uint16_t length, pbuf_type type)
{
    /* Allocate the 'head' of the packet chain */
    struct pbuf *p_head = memp_malloc(MEMP_PBUF_POOL);
    if (!p_head) {
        return (NULL);
    }
    struct rte_mbuf *m_head = packetio_memp_pbuf_to_mbuf(p_head);

    /* Fill in pbuf details */
    p_head->next = NULL;
    p_head->payload = rte_pktmbuf_mtod(m_head, void *);
    p_head->tot_len = rte_pktmbuf_pkt_len(m_head) = length;
    p_head->len = (rte_pktmbuf_data_len(m_head)
                   = LWIP_MIN(length, m_head->buf_len - _get_header_offset(layer)));
    p_head->type = type;
    p_head->flags = 0;
    p_head->ref = 1;

    /* Allocate any tail, if needed */
    struct pbuf *prev = p_head;
    uint16_t remainder = length - p_head->len;
    while (remainder > 0) {
        struct pbuf *p = memp_malloc(MEMP_PBUF_POOL);
        if (!p) {
            pbuf_free(prev);
            return (NULL);
        }
        struct rte_mbuf *m = packetio_memp_pbuf_to_mbuf(p);
        p->next = NULL;
        p->payload = rte_pktmbuf_mtod(m, void *);
        p->tot_len = rte_pktmbuf_pkt_len(m) = remainder;
        p->len = rte_pktmbuf_data_len(m) = LWIP_MIN(remainder, m->buf_len);
        p->type = type;
        p->flags = 0;
        p->ref = 1;

        remainder -= p->len;
        prev->next = p;
        prev = p;
    }

    return (p_head);
}

struct pbuf * _allocate_native_pbuf(uint16_t length, pbuf_type type)
{
    struct pbuf *p = memp_malloc(MEMP_PBUF);
    if (!p) {
        LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("pbuf_alloc: Could not allocate MEMP_PBUF for PBUF_%s.\n",
                     (type == PBUF_ROM) ? "ROM" : "REF"));
        return (NULL);
    }

    /* Expect callers to fill in this data */
    p->next = NULL;
    p->payload = NULL;
    p->tot_len = p->len = length;
    p->type = type;
    p->flags = 0;
    p->ref = 1;

    return (p);
}

struct pbuf *pbuf_alloc(pbuf_layer layer, u16_t length, pbuf_type type)
{
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F")\n", length));

    struct pbuf *to_return = NULL;

    switch (type) {
    case PBUF_POOL:
    case PBUF_RAM:
        to_return = _allocate_dpdk_backed_pbuf(layer, length, type);
        break;
    case PBUF_ROM:
    case PBUF_REF:
        to_return = _allocate_native_pbuf(length, type);
        break;
    }

    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_alloc(length=%"U16_F") == %p\n", length, (void *)to_return));

    return (to_return);
}

/**
 * @ingroup pbuf
 * Shrink a pbuf chain to a desired length.
 *
 * @param p pbuf to shrink.
 * @param new_len desired new length of pbuf chain
 *
 * Depending on the desired length, the first few pbufs in a chain might
 * be skipped and left unchanged. The new last pbuf in the chain will be
 * resized, and any remaining pbufs will be freed.
 *
 * @note If the pbuf is ROM/REF, only the ->tot_len and ->len fields are adjusted.
 * @note May not be called on a packet queue.
 *
 * @note Despite its name, pbuf_realloc cannot grow the size of a pbuf (chain).
 */
void
pbuf_realloc(struct pbuf *p, u16_t new_len)
{
    LWIP_ASSERT("pbuf_realloc: p != NULL", p != NULL);

    /* desired length larger than current length? */
    if (new_len >= p->tot_len) {
        /* enlarging not yet supported */
        return;
    }

    unsigned grow = new_len - p->tot_len;

    /* Walk the chain until we find the buf that needs to shrink */
    unsigned rem_len = new_len;
    struct pbuf *q = p;
    while (rem_len > q->len) {
        rem_len -= q->len;
        /* decrease total length indicator */
        LWIP_ASSERT("grow < max_u16_t", grow < 0xffff);
        q->tot_len += (u16_t)grow;
        /* proceed to next pbuf in chain */
        q = q->next;
        LWIP_ASSERT("pbuf_realloc: q != NULL", q != NULL);
    }

    /* we have now reached the new last pbuf (in q) */
    /* rem_len == desired length for pbuf q */

    /* shrink allocated memory for PBUF_RAM */
    /* (other types merely adjust their length fields */
    if (q->type == PBUF_RAM || q->type == PBUF_POOL) {
        if (rte_pktmbuf_trim(packetio_memp_pbuf_to_mbuf(q), -grow) != 0) {
            LWIP_ASSERT("rte_pktmbuf_trim failed", false);
        }
    }

    q->len = rem_len;
    q->tot_len = q->len;

    /* any remaining pbufs in chain? */
    if (q->next != NULL) {
        /* free remaining pbufs in chain */
        pbuf_free(q->next);
    }
    /* q is last packet in chain */
    q->next = NULL;
}

static int pbuf_header_impl(struct pbuf *p, int16_t header_size_increment, bool force)
{
    LWIP_ASSERT("p != NULL", p != NULL);
    if ((header_size_increment == 0) || (p == NULL)) {
        return 0;
    }

    uint16_t increment_magnitude = 0;
    if (header_size_increment < 0) {
        increment_magnitude = (u16_t)-header_size_increment;
        /* Check that we aren't going to move off the end of the pbuf */
        LWIP_ERROR("increment_magnitude <= p->len", (increment_magnitude <= p->len), return 1;);
    } else {
        increment_magnitude = (u16_t)header_size_increment;
    }

    void *payload = NULL;
    switch (p->type) {
    case PBUF_POOL:
    case PBUF_RAM:
        if (header_size_increment < 0) {
            payload = rte_pktmbuf_adj(packetio_memp_pbuf_to_mbuf(p), increment_magnitude);
        } else {
            payload = rte_pktmbuf_prepend(packetio_memp_pbuf_to_mbuf(p), increment_magnitude);
        }
        if (!payload) {
            LWIP_DEBUGF( PBUF_DEBUG | LWIP_DBG_TRACE,
                         ("pbuf_header: failed as %p < %p (not enough space for new header size)\n",
                          (void *)p->payload, (void *)((u8_t *)p + sizeof(struct pbuf))));
            return (1);
        }
        p->payload = payload;
        break;
    case PBUF_ROM:
    case PBUF_REF:
        /* hide a header in the payload? */
        if ((header_size_increment < 0) && (increment_magnitude <= p->len)) {
            /* increase payload pointer */
            p->payload = (u8_t *)p->payload - header_size_increment;
        } else if ((header_size_increment > 0) && force) {
            p->payload = (u8_t *)p->payload - header_size_increment;
        } else {
            /* cannot expand payload to front (yet!)
             * bail out unsuccessfully */
            return (1);
        }
        break;
    default:
        /* Unknown type */
        LWIP_ASSERT("bad pbuf type", 0);
        return (1);
    }

    p->len += header_size_increment;
    p->tot_len += header_size_increment;

    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_header: old %p new %p (%"S16_F")\n",
                                              (void *)payload, (void *)p->payload,
                                              header_size_increment));

    return 0;
}

/**
 * Adjusts the payload pointer to hide or reveal headers in the payload.
 *
 * Adjusts the ->payload pointer so that space for a header
 * (dis)appears in the pbuf payload.
 *
 * The ->payload, ->tot_len and ->len fields are adjusted.
 *
 * @param p pbuf to change the header size.
 * @param header_size_increment Number of bytes to increment header size which
 * increases the size of the pbuf. New space is on the front.
 * (Using a negative value decreases the header size.)
 * If hdr_size_inc is 0, this function does nothing and returns successful.
 *
 * PBUF_ROM and PBUF_REF type buffers cannot have their sizes increased, so
 * the call will fail. A check is made that the increase in header size does
 * not move the payload pointer in front of the start of the buffer.
 * @return non-zero on failure, zero on success.
 *
 */
u8_t
pbuf_header(struct pbuf *p, s16_t header_size_increment)
{
    return pbuf_header_impl(p, header_size_increment, false);
}

/**
 * Same as pbuf_header but does not check if 'header_size > 0' is allowed.
 * This is used internally only, to allow PBUF_REF for RX.
 */
u8_t
pbuf_header_force(struct pbuf *p, s16_t header_size_increment)
{
    return pbuf_header_impl(p, header_size_increment, true);
}

/**
 * @ingroup pbuf
 * Dereference a pbuf chain or queue and deallocate any no-longer-used
 * pbufs at the head of this chain or queue.
 *
 * Decrements the pbuf reference count. If it reaches zero, the pbuf is
 * deallocated.
 *
 * For a pbuf chain, this is repeated for each pbuf in the chain,
 * up to the first pbuf which has a non-zero reference count after
 * decrementing. So, when all reference counts are one, the whole
 * chain is free'd.
 *
 * @param p The pbuf (chain) to be dereferenced.
 *
 * @return the number of pbufs that were de-allocated
 * from the head of the chain.
 *
 * @note MUST NOT be called on a packet queue (Not verified to work yet).
 * @note the reference counter of a pbuf equals the number of pointers
 * that refer to the pbuf (or into the pbuf).
 *
 * @internal examples:
 *
 * Assuming existing chains a->b->c with the following reference
 * counts, calling pbuf_free(a) results in:
 *
 * 1->2->3 becomes ...1->3
 * 3->3->3 becomes 2->3->3
 * 1->1->2 becomes ......1
 * 2->1->1 becomes 1->1->1
 * 1->1->1 becomes .......
 *
 */
uint8_t pbuf_free(struct pbuf *p)
{
    if (p == NULL) {
        LWIP_ASSERT("p != NULL", p != NULL);
        /* if assertions are disabled, proceed with debug output */
        LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_LEVEL_SERIOUS,
                    ("pbuf_free(p == NULL) was called.\n"));
        return 0;
    }
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free(%p)\n", (void *)p));

    PERF_START;

    LWIP_ASSERT("pbuf_free: sane type",
                p->type == PBUF_RAM || p->type == PBUF_ROM ||
                p->type == PBUF_REF || p->type == PBUF_POOL);

    /* de-allocate all consecutive pbufs from the head of the chain that
     * obtain a zero reference count after decrementing*/

    uint8_t count = 0;
    while (p) {
        struct pbuf *q = NULL;

        if (atomic_fetch_sub_explicit((atomic_ushort *)&p->ref, 1,
                                      memory_order_acq_rel) == 1) {
            q = p->next;
            switch (p->type) {
            case PBUF_POOL:
            case PBUF_RAM:
                memp_free(MEMP_PBUF_POOL, p);
                break;
            case PBUF_ROM:
            case PBUF_REF:
                memp_free(MEMP_PBUF, p);
            }
            count++;
        }
        p = q;
    }

    PERF_STOP("pbuf_free");

    return (count);
}

/**
 * Count number of pbufs in a chain
 *
 * @param p first pbuf of chain
 * @return the number of pbufs in a chain
 */
u16_t
pbuf_clen(const struct pbuf *p)
{
    u16_t len;

    len = 0;
    while (p != NULL) {
        ++len;
        p = p->next;
    }
    return len;
}

/**
 * @ingroup pbuf
 * Increment the reference count of the pbuf.
 *
 * @param p pbuf to increase reference counter of
 *
 */
void
pbuf_ref(struct pbuf *p)
{
    /* pbuf given? */
    if (p != NULL) {
        switch (p->type) {
        case PBUF_POOL:
        case PBUF_RAM:
            rte_pktmbuf_refcnt_update(packetio_memp_pbuf_to_mbuf(p), 1);
            /* fall through */
        case PBUF_ROM:
        case PBUF_REF:
            atomic_fetch_add_explicit((atomic_ushort *)&p->ref, 1,
                                      memory_order_acq_rel);
            LWIP_ASSERT("pbuf ref overflow", p->ref > 0);
            break;
        default:
            /* Unknown type */
            LWIP_ASSERT("bad pbuf type", 0);
        }
    }
}

/**
 * @ingroup pbuf
 * Concatenate two pbufs (each may be a pbuf chain) and take over
 * the caller's reference of the tail pbuf.
 *
 * @note The caller MAY NOT reference the tail pbuf afterwards.
 * Use pbuf_chain() for that purpose.
 *
 * @see pbuf_chain()
 */
void
pbuf_cat(struct pbuf *h, struct pbuf *t)
{
    struct pbuf *p;

    LWIP_ERROR("(h != NULL) && (t != NULL) (programmer violates API)",
               ((h != NULL) && (t != NULL)), return;);

    /* proceed to last pbuf of chain */
    for (p = h; p->next != NULL; p = p->next) {
        /* add total length of second chain to all totals of first chain */
        p->tot_len += t->tot_len;
    }
    /* { p is last pbuf of first h chain, p->next == NULL } */
    LWIP_ASSERT("p->tot_len == p->len (of last pbuf in chain)", p->tot_len == p->len);
    LWIP_ASSERT("p->next == NULL", p->next == NULL);
    /* add total length of second chain to last pbuf total of first chain */
    p->tot_len += t->tot_len;
    /* chain last pbuf of head (p) with first of tail (t) */
    p->next = t;
    /* p->next now references t, but the caller will drop its reference to t,
     * so netto there is no change to the reference count of t.
     */
}

/**
 * @ingroup pbuf
 * Chain two pbufs (or pbuf chains) together.
 *
 * The caller MUST call pbuf_free(t) once it has stopped
 * using it. Use pbuf_cat() instead if you no longer use t.
 *
 * @param h head pbuf (chain)
 * @param t tail pbuf (chain)
 * @note The pbufs MUST belong to the same packet.
 * @note MAY NOT be called on a packet queue.
 *
 * The ->tot_len fields of all pbufs of the head chain are adjusted.
 * The ->next field of the last pbuf of the head chain is adjusted.
 * The ->ref field of the first pbuf of the tail chain is adjusted.
 *
 */
void
pbuf_chain(struct pbuf *h, struct pbuf *t)
{
    pbuf_cat(h, t);
    /* t is now referenced by h */
    pbuf_ref(t);
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_chain: %p references %p\n",
                                              (void *)h, (void *)t));
}

/**
 * @ingroup pbuf
 * Create PBUF_RAM copies of pbufs.
 *
 * Used to queue packets on behalf of the lwIP stack, such as
 * ARP based queueing.
 *
 * @note You MUST explicitly use p = pbuf_take(p);
 *
 * @note Only one packet is copied, no packet queue!
 *
 * @param p_to pbuf destination of the copy
 * @param p_from pbuf source of the copy
 *
 * @return ERR_OK if pbuf was copied
 *         ERR_ARG if one of the pbufs is NULL or p_to is not big
 *                 enough to hold p_from
 */
err_t
pbuf_copy(struct pbuf *p_to, const struct pbuf *p_from)
{
    u16_t offset_to=0, offset_from=0, len;

    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy(%p, %p)\n",
                                              (const void*)p_to, (const void*)p_from));

    /* is the target big enough to hold the source? */
    LWIP_ERROR("pbuf_copy: target not big enough to hold source",
               ((p_to != NULL) && (p_from != NULL) && (p_to->tot_len >= p_from->tot_len)),
               return ERR_ARG;);

    /* iterate through pbuf chain */
    do {
        /* copy one part of the original chain */
        if ((p_to->len - offset_to) >= (p_from->len - offset_from)) {
            /* complete current p_from fits into current p_to */
            len = p_from->len - offset_from;
        } else {
            /* current p_from does not fit into current p_to */
            len = p_to->len - offset_to;
        }
        rte_memcpy((u8_t*)p_to->payload + offset_to, (u8_t*)p_from->payload + offset_from, len);
        offset_to += len;
        offset_from += len;
        LWIP_ASSERT("offset_to <= p_to->len", offset_to <= p_to->len);
        LWIP_ASSERT("offset_from <= p_from->len", offset_from <= p_from->len);
        if (offset_from >= p_from->len) {
            /* on to next p_from (if any) */
            offset_from = 0;
            p_from = p_from->next;
        }
        if (offset_to == p_to->len) {
            /* on to next p_to (if any) */
            offset_to = 0;
            p_to = p_to->next;
            LWIP_ERROR("p_to != NULL", (p_to != NULL) || (p_from == NULL) , return ERR_ARG;);
        }

        if ((p_from != NULL) && (p_from->len == p_from->tot_len)) {
            /* don't copy more than one packet! */
            LWIP_ERROR("pbuf_copy() does not allow packet queues!",
                       (p_from->next == NULL), return ERR_VAL;);
        }
        if ((p_to != NULL) && (p_to->len == p_to->tot_len)) {
            /* don't copy more than one packet! */
            LWIP_ERROR("pbuf_copy() does not allow packet queues!",
                       (p_to->next == NULL), return ERR_VAL;);
        }
    } while (p_from);
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_copy: end of chain reached.\n"));
    return ERR_OK;
}

/**
 * @ingroup pbuf
 * Copy (part of) the contents of a packet buffer
 * to an application supplied buffer.
 *
 * @param buf the pbuf from which to copy data
 * @param dataptr the application supplied buffer
 * @param len length of data to copy (dataptr must be big enough). No more
 * than buf->tot_len will be copied, irrespective of len
 * @param offset offset into the packet buffer from where to begin copying len bytes
 * @return the number of bytes copied, or 0 on failure
 */
u16_t
pbuf_copy_partial(const struct pbuf *buf, void *dataptr, u16_t len, u16_t offset)
{
    const struct pbuf *p;
    u16_t left;
    u16_t buf_copy_len;
    u16_t copied_total = 0;

    LWIP_ERROR("pbuf_copy_partial: invalid buf", (buf != NULL), return 0;);
    LWIP_ERROR("pbuf_copy_partial: invalid dataptr", (dataptr != NULL), return 0;);

    left = 0;

    if ((buf == NULL) || (dataptr == NULL)) {
        return 0;
    }

    /* Note some systems use byte copy if dataptr or one of the pbuf payload pointers are unaligned. */
    for (p = buf; len != 0 && p != NULL; p = p->next) {
        if ((offset != 0) && (offset >= p->len)) {
            /* don't copy from this buffer -> on to the next */
            offset -= p->len;
        } else {
            /* copy from this buffer. maybe only partially. */
            buf_copy_len = p->len - offset;
            if (buf_copy_len > len) {
                buf_copy_len = len;
            }
            /* copy the necessary parts of the buffer */
            MEMCPY(&((char*)dataptr)[left], &((char*)p->payload)[offset], buf_copy_len);
            copied_total += buf_copy_len;
            left += buf_copy_len;
            len -= buf_copy_len;
            offset = 0;
        }
    }
    return copied_total;
}

/* XXX: add these later */
volatile uint8_t pbuf_free_ooseq_pending = 0;

void pbuf_free_ooseq()
{
    LWIP_DEBUGF(PBUF_DEBUG | LWIP_DBG_TRACE, ("pbuf_free_ooseq: freeing out-of-sequence pbufs\n"));
}
