#include <array>
#include <cassert>

#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/stack/dpdk/net_interface.h"
#include "packetio/stack/dpdk/net_interface_rx.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk::netif_rx_strategy {

/***
 * Queueing strategy functions
 ***/

static err_t net_interface_rx_notify(pbuf *p, netif* netintf)
{
    assert(p);
    assert(netintf);

    auto ifp = reinterpret_cast<net_interface*>(netintf->state);
    return (ifp->handle_rx_notify());
}

static std::string recvq_name(std::string_view prefix, uint16_t idx)
{
    return ("rx_queue_" + std::string(prefix) + std::to_string(idx));
}

queueing::queueing(std::string_view if_prefix, uint16_t if_idx, uint16_t port_idx)
    : m_notify(false)
{
    /* rx_queue_size must be a power of 2 */
    static_assert(rx_queue_size != 0 && (rx_queue_size & (rx_queue_size - 1)) == 0);

    m_queue = std::unique_ptr<rte_ring, rte_ring_deleter>
        ((rte_ring_create(recvq_name(if_prefix, if_idx).c_str(), rx_queue_size,
                          rte_eth_dev_socket_id(port_idx), RING_F_SC_DEQ)));
}


err_t queueing::handle_rx_notify(netif* netintf)
{
    static constexpr size_t rx_burst_size = 32;
    std::array<pbuf*, rx_burst_size> pbufs;

    do {
        auto nb_pbufs = rte_ring_dequeue_burst(m_queue.get(),
                                               reinterpret_cast<void**>(pbufs.data()),
                                               rx_burst_size, nullptr);

        for (size_t i = 0; i < nb_pbufs; i++) {
            /* Note: ethernet_input _always_ return ERR_OK */
            ethernet_input(pbufs[i], netintf);
        }
    } while (!rte_ring_empty(m_queue.get()));

    m_notify.clear(std::memory_order_release);
    return (ERR_OK);
}

err_t queueing::handle_rx(pbuf* p, netif* netintf)
{
    if (rte_ring_enqueue(m_queue.get(), p) != 0) {
        ICP_LOG(ICP_LOG_WARNING, "Receive queue on %c%c%u is full; packet dropped!\n",
            netintf->name[0], netintf->name[1], netintf->num);
        return (ERR_BUF);
    }
    if (!m_notify.test_and_set(std::memory_order_acquire)) {
        tcpip_inpkt(p, netintf, net_interface_rx_notify);
    }
    return (ERR_OK);
}

/***
 * Direct strategy functions
 ***/

err_t direct::handle_rx_notify(netif* netintf)
{
    /* Should never be called */
    ICP_LOG(ICP_LOG_ERROR, "Calling useless rx notify function on %c%c%u\n",
            netintf->name[0], netintf->name[1], netintf->num);
    return (ERR_IF);
}

err_t direct::handle_rx(pbuf* p, netif* netintf)
{
    return (ethernet_input(p, netintf));
}

}
