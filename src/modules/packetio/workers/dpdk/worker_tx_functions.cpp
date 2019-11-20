#include "core/op_log.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/workers/dpdk/worker_queues.h"
#include "packetio/workers/dpdk/worker_tx_functions.h"

namespace openperf::packetio::dpdk::worker {

/* Copy mbuf header and packet data */
static void eal_mbuf_copy_seg(rte_mbuf *dst, const rte_mbuf* src)
{
    assert(dst);
    assert(src);

    dst->data_off = src->data_off;
    dst->port = src->port;
    dst->ol_flags = src->ol_flags;
    dst->packet_type = src->packet_type;
    dst->pkt_len = src->pkt_len;
    dst->data_len = src->data_len;
    dst->vlan_tci = src->vlan_tci;
    dst->vlan_tci_outer = src->vlan_tci_outer;
    dst->timestamp = src->timestamp;
    dst->udata64 = src->udata64;
    dst->tx_offload = src->tx_offload;
    dst->timesync = src->timesync;

    rte_memcpy(rte_pktmbuf_mtod(dst, void*),
               rte_pktmbuf_mtod(src, const void*),
               rte_pktmbuf_data_len(src));
}

/* Make a complete copy of an mbuf chain with mbufs from the same pool as the source */
static rte_mbuf* eal_mbuf_copy_chain(const rte_mbuf* src_head)
{
    assert(src_head);

    rte_mbuf* dst_head = rte_pktmbuf_alloc(src_head->pool);
    if (!dst_head) return (nullptr);
    eal_mbuf_copy_seg(dst_head, src_head);

    /*
     * Ugh.  The mbuf free function sanity checks the mbufs, so it can panic
     * if pkt_len and nb_segs is not accurate.  Hence, we have to update those
     * fields as we go along so that we can free the mbuf if there is an error.
     */
    rte_pktmbuf_pkt_len(dst_head) = rte_pktmbuf_data_len(dst_head);

    const rte_mbuf* src = src_head;
    rte_mbuf* dst = dst_head;
    while (src->next != nullptr) {
        src = src->next;
        auto next = rte_pktmbuf_alloc(src->pool);
        if (!next) {
            rte_pktmbuf_free(dst_head);
            return (nullptr);
        }
        eal_mbuf_copy_seg(next, src);
        dst->next = next;
        dst = dst->next;
        dst_head->nb_segs++;
        rte_pktmbuf_pkt_len(dst_head) += rte_pktmbuf_data_len(dst);
    }

    rte_mbuf_sanity_check(dst_head, true);

    return (dst_head);
}

static uint16_t transmit(uint16_t port_idx, uint16_t queue_idx,
                         struct rte_mbuf* mbufs[], uint16_t nb_mbufs)
{
    auto sent = rte_eth_tx_burst(port_idx, queue_idx, mbufs, nb_mbufs);

    OP_LOG(OP_LOG_TRACE, "Transmitted %u of %u packet%s on %u:%u\n",
            sent, nb_mbufs, sent > 1 ? "s" : "", port_idx, queue_idx);

    return (sent);
}


uint16_t tx_copy_direct(int port_idx, uint32_t, struct rte_mbuf* mbufs[], uint16_t nb_mbufs)
{
    uint16_t sent = 0;

    for (uint16_t i = 0; i < nb_mbufs; i++) {
        /*
         * Note: caller expects the driver to free the mbuf when transmitted, hence
         * we free the original mbuf here and the driver frees the copy.
         */
        auto copy = eal_mbuf_copy_chain(mbufs[i]);
        rte_pktmbuf_free(mbufs[i]);
        if (!copy || !transmit(port_idx, 0, &copy, 1)) {
            return (sent);
        }
        sent++;
    }

    return (sent);
}

uint16_t tx_copy_queued(int port_idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs)
{
    uint16_t sent = 0;
    auto& queues = worker::port_queues::instance();

    for (uint16_t i = 0; i < nb_mbufs; i++) {
        /*
         * Note: caller expects the driver to free the mbuf when transmitted, hence
         * we free the original mbuf here and the driver frees the copy.
         */
        auto copy = eal_mbuf_copy_chain(mbufs[i]);
        rte_pktmbuf_free(mbufs[i]);
        if (!copy || !queues[port_idx].tx(hash)->enqueue(&copy, 1)) {
            return (sent);
        }
        sent++;
    }

    return (sent);
}

uint16_t tx_direct(int port_idx, uint32_t, struct rte_mbuf* mbufs[], uint16_t nb_mbufs)
{
    return (transmit(port_idx, 0, mbufs, nb_mbufs));
}

uint16_t tx_queued(int port_idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs)
{
    auto& queues = worker::port_queues::instance();
    return (queues[port_idx].tx(hash)->enqueue(mbufs, nb_mbufs));
}

uint16_t tx_dummy(int port_idx, uint32_t, struct rte_mbuf**, uint16_t)
{
    OP_LOG(OP_LOG_WARNING, "Dummy TX function called for port %u; "
            "no packet transmitted!\n", port_idx);
    return (0);
}

}
