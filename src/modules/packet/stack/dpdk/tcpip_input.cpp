#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>

#include "lwip/netif.h"
#include "lwip/snmp.h"
#include "lwip/tcpip.h"

#include "rte_errno.h"

#include "packet/stack/dpdk/pbuf_utils.h"
#include "packet/stack/dpdk/tcpip_input.hpp"
#include "packetio/drivers/dpdk/mbuf_metadata.hpp"

namespace openperf::packet::stack::dpdk {

namespace impl {

/* Receive worker context */
err_t tcpip_input_direct::inject(netif* ifp, rte_mbuf* mbuf)
{
    return (ifp->input(packet_stack_pbuf_synchronize(mbuf), ifp));
}

/* Stack context */
void process_input_packets(void* arg)
{
    static constexpr auto rx_burst_size = 32U;
    std::array<pbuf*, rx_burst_size> pbufs;

    auto* q = reinterpret_cast<tcpip_input_queue*>(arg);
    q->ack();

    uint16_t nb_pbufs = 0;

    do {
        nb_pbufs = q->dequeue(pbufs.data(), pbufs.size());

        /* Inject each packet into the stack. */
        std::for_each(pbufs.data(), pbufs.data() + nb_pbufs, [&](auto* pbuf) {
            auto* ifp = packetio::dpdk::mbuf_tag_get<netif>(
                packet_stack_pbuf_to_mbuf(pbuf));
            assert(ifp != nullptr);
            if (ifp->input(pbuf, ifp) != ERR_OK) {
                MIB2_STATS_NETIF_INC_ATOMIC(ifp, ifindiscards);
                pbuf_free(pbuf);
            }
        });
    } while (nb_pbufs);
}

tcpip_input_queue::tcpip_input_queue()
{
    /* rx queue size must be a power of 2 */
    static_assert(rx_queue_size != 0
                  && (rx_queue_size & (rx_queue_size - 1)) == 0);

    /*
     * Since our stack is currently single threaded, we can use the single
     * consumer dequeue
     */
    m_queue = std::unique_ptr<rte_ring, rte_ring_deleter>(
        (rte_ring_create(ring_name,
                         rx_queue_size,
                         static_cast<int>(rte_socket_id()),
                         RING_F_SC_DEQ)));
    if (!m_queue) {
        throw std::runtime_error("Could not allocate stack input queue: "
                                 + std::string(rte_strerror(rte_errno)));
    }
}

void tcpip_input_queue::ack() { m_notify.clear(std::memory_order_release); }

uint16_t tcpip_input_queue::dequeue(pbuf* packets[], uint16_t max_packets)
{
    return (rte_ring_dequeue_burst(m_queue.get(),
                                   reinterpret_cast<void**>(packets),
                                   max_packets,
                                   nullptr));
}

err_t tcpip_input_queue::inject(netif* ifp, rte_mbuf* packet)
{
    packetio::dpdk::mbuf_tag_set(packet, ifp);

    /* XXX: Important to synchronize in the worker thread context */
    if (rte_ring_enqueue(m_queue.get(), packet_stack_pbuf_synchronize(packet))
        != 0) {
        OP_LOG(OP_LOG_WARNING,
               "TCP/IP stack receive queue is full; dropping packet!\n");
        MIB2_STATS_NETIF_INC_ATOMIC(ifp, ifindiscards);
        return (ERR_MEM);
    }

    if (!m_notify.test_and_set(std::memory_order_acq_rel)) {
        /* We really can't lose this notification, so keep trying! */
        while (tcpip_try_callback(process_input_packets, this) != ERR_OK) {
            rte_pause();
        }
    }

    return (ERR_OK);
}

} // namespace impl

void tcpip_input::init()
{
    if (rte_lcore_count() > 2) { m_input.emplace<impl::tcpip_input_queue>(); }
}

void tcpip_input::fini() { m_input.emplace<impl::tcpip_input_direct>(); }

err_t tcpip_input::inject(netif* netif, rte_mbuf* packet)
{
    return (std::visit([&](auto& impl) { return (impl.inject(netif, packet)); },
                       m_input));
}

} // namespace openperf::packet::stack::dpdk
