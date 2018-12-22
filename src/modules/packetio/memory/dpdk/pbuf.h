#ifndef _ICP_PACKETIO_MEMORY_DPDK_PBUF_H_
#define _ICP_PACKETIO_MEMORY_DPDK_PBUF_H_

#include "lwip/pbuf.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/memory/dpdk/memp.h"

/*
 * Update the pbuf headers inside the given mbuf chain so that the
 * mbufs may be used as pbufs by the stack.
 */
inline struct pbuf *packetio_pbuf_synchronize(struct rte_mbuf *m_head)
{
    struct rte_mbuf *m = m_head;
    do {
        struct pbuf *p = packetio_memp_mbuf_to_pbuf(m);
        p->next = (m->next == NULL ? NULL : packetio_memp_mbuf_to_pbuf(m->next));
        p->payload = rte_pktmbuf_mtod(m, void *);
        p->tot_len = rte_pktmbuf_pkt_len(m);
        p->len = rte_pktmbuf_data_len(m);
        p->type = PBUF_POOL;
        p->flags = 0;
        p->ref = m->refcnt;
    } while ((m = m->next) != NULL);

    return (packetio_memp_mbuf_to_pbuf(m_head));
}

#endif /* _ICP_PACKETIO_MEMORY_DPDK_PBUF_H_ */
