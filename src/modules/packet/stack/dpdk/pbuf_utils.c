#include <assert.h>
#include <stdbool.h>

#include "lwip/netif.h"

#include "packet/stack/dpdk/pbuf_utils.h"

#if RTE_MBUF_PRIV_ALIGN != MEM_ALIGNMENT
#error "RTE_MBUF_PRIV_ALIGN and lwip's MEM_ALIGNMENT msut be equal"
#endif

/*
 * We stuff the pbuf header in the DPDK mbuf private data area, so
 * that the stack can use DPDK mbufs directly without any additional
 * overhead.  This also means we can translate pbuf <--> mbuf by
 * just adding or subtracting an offset.
 * Note: this only works because the headers are nicely aligned.
 */
struct rte_mbuf* packet_stack_pbuf_to_mbuf(const struct pbuf* pbuf)
{
    return (pbuf != NULL ? (struct rte_mbuf*)((uintptr_t)(pbuf)
                                              - sizeof(struct rte_mbuf))
                         : NULL);
}

struct pbuf* packet_stack_mbuf_to_pbuf(const struct rte_mbuf* mbuf)
{
    return (mbuf != NULL
                ? (struct pbuf*)((uintptr_t)(mbuf) + sizeof(struct rte_mbuf))
                : NULL);
}

/*
 * Update the pbuf headers inside the given mbuf chain so that the
 * mbufs may be used as pbufs by the stack.
 *
 * Note: Both DPDK and lwIP support chaining buffers together.  Both
 * also contain data fields for the length of the chain and the length
 * of the segment.  However, DPDK chains only have the correct chain
 * length value on the chain head.  LwIP wants the chain length to
 * be correct for all buffers in the chain, so that it can easily
 * split chains into sub chains.  As a result, we have to keep track
 * of the chain length when we convert DPDK metadata to lwIP metadata.
 */
struct pbuf* packet_stack_pbuf_synchronize(struct rte_mbuf* m_head)
{
    struct rte_mbuf* m = m_head;
    uint16_t m_chain_len = rte_pktmbuf_pkt_len(m_head);
    do {
        struct pbuf* p = packet_stack_mbuf_to_pbuf(m);
        p->next = packet_stack_mbuf_to_pbuf(m->next);
        p->payload = rte_pktmbuf_mtod(m, void*);
        p->tot_len = m_chain_len;
        p->len = rte_pktmbuf_data_len(m);
        p->type_internal = (uint8_t)PBUF_POOL;
        if (RTE_MBUF_HAS_EXTBUF(m) || (m->ol_flags & IND_ATTACHED_MBUF)) {
            /* If mbuf data is external/indirect (e.g. mbuf was cloned for
             * multicast dispatch) the pbuf header is not contiguous to the pbuf
             * data.  Clearing this flag disables additional error checking.
             */
            p->type_internal &= ~(PBUF_TYPE_FLAG_STRUCT_DATA_CONTIGUOUS);
        }
        p->flags = 0;
        p->ref = 1;
        p->if_idx = NETIF_NO_INDEX;
        m_chain_len -= rte_pktmbuf_data_len(m);
    } while ((m = m->next) != NULL);

    return (packet_stack_mbuf_to_pbuf(m_head));
}

/*
 * Internally, lwIP will link multiple pbuf chains together.  As a
 * result, p->next might not be null at the end of a chain.  Verify
 * a chain continues by comparing chain length to segment length.
 * They are only equal on the *last* segment of a chain.
 */
static bool pbuf_has_next(const struct pbuf* p)
{
    return (p->tot_len != p->len && p->next);
}

static uint16_t pbuf_actual_clen(const struct pbuf* p)
{
    uint16_t count = 0;
    do {
        count++;
    } while ((p = (pbuf_has_next(p) ? p->next : NULL)) != NULL);

    return (count);
}

struct rte_mbuf* packet_stack_mbuf_synchronize(struct pbuf* p_head)
{
    struct pbuf* p = p_head;
    uint16_t nb_segs = pbuf_actual_clen(p);
    do {
        struct rte_mbuf* m = packet_stack_pbuf_to_mbuf(p);
        m->data_off = (uintptr_t)(p->payload) - (uintptr_t)(m->buf_addr);
        m->next =
            (pbuf_has_next(p) ? packet_stack_pbuf_to_mbuf(p->next) : NULL);
        m->nb_segs = nb_segs--;
        rte_pktmbuf_pkt_len(m) = p->tot_len;
        rte_pktmbuf_data_len(m) = p->len;
    } while ((p = (pbuf_has_next(p) ? p->next : NULL)) != NULL);

    assert(nb_segs == 0);

    return (packet_stack_pbuf_to_mbuf(p_head));
}

uint16_t packet_stack_pbuf_data_available(const struct pbuf* p)
{
    struct rte_mbuf* m = packet_stack_pbuf_to_mbuf(p);
    uint16_t headroom = packet_stack_pbuf_header_available(p);
    return (m->buf_len > headroom ? m->buf_len - headroom : 0);
}

uint16_t packet_stack_pbuf_header_available(const struct pbuf* p)
{
    struct rte_mbuf* m = packet_stack_pbuf_to_mbuf(p);
    uint16_t payload_offset =
        (uintptr_t)(p->payload) - (uintptr_t)(m->buf_addr);
    return (payload_offset);
}
