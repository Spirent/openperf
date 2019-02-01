#include "lwip/netif.h"

#include "packetio/memory/dpdk/pbuf_utils.h"

/*
 * We stuff the pbuf header in the DPDK mbuf private data area, so
 * that the stack can use DPDK mbufs directly without any additional
 * overhead.  This also means we can translate pbuf <--> mbuf by
 * just adding or subtracting an offset.
 * Note: this only works because the headers are nicely aligned.
 */
struct rte_mbuf * packetio_memory_pbuf_to_mbuf(const struct pbuf *pbuf)
{
    return (pbuf != NULL
            ? (struct rte_mbuf *)((uintptr_t)(pbuf) - sizeof(struct rte_mbuf))
            : NULL);
}

struct pbuf * packetio_memory_mbuf_to_pbuf(const struct rte_mbuf *mbuf)
{
    return (mbuf != NULL
            ? (struct pbuf *)((uintptr_t)(mbuf) + sizeof(struct rte_mbuf))
            : NULL);
}

/*
 * Update the pbuf headers inside the given mbuf chain so that the
 * mbufs may be used as pbufs by the stack.
 * We increment the mbuf refcnt, because that refcnt needs to be
 * equal to 1 when we return the mbuf to the mbuf memory pool.
 */
struct pbuf * packetio_memory_pbuf_synchronize(struct rte_mbuf *m_head)
{
    struct rte_mbuf *m = m_head;
    do {
        struct pbuf *p = packetio_memory_mbuf_to_pbuf(m);
        p->next = (m->next == NULL ? NULL : packetio_memory_mbuf_to_pbuf(m->next));
        p->payload = rte_pktmbuf_mtod(m, void *);
        p->tot_len = rte_pktmbuf_pkt_len(m);
        p->len = rte_pktmbuf_data_len(m);
        p->type_internal = (uint8_t)PBUF_POOL;
        p->flags = 0;
        p->ref = 1;
        p->if_idx = NETIF_NO_INDEX;
    } while ((m = m->next) != NULL);

    return (packetio_memory_mbuf_to_pbuf(m_head));
}

struct rte_mbuf * packetio_memory_mbuf_synchronize(struct pbuf *p_head)
{
    struct pbuf* p = p_head;
    do {
        struct rte_mbuf* m = packetio_memory_pbuf_to_mbuf(p);
        m->data_off = (uintptr_t)(p->payload) - (uintptr_t)(m->buf_addr);
        m->next = (p->next == NULL ? NULL : packetio_memory_pbuf_to_mbuf(p->next));
        m->nb_segs = pbuf_clen(p);
        rte_pktmbuf_pkt_len(m) = p->tot_len;
        rte_pktmbuf_data_len(m) = p->len;
    } while ((p = p->next) != NULL);

    return (packetio_memory_pbuf_to_mbuf(p_head));
}
