#ifndef _OP_PACKETIO_DPDK_NET_INTERFACE_RX_HPP_
#define _OP_PACKETIO_DPDK_NET_INTERFACE_RX_HPP_

#include <memory>
#include <string>

#include "lwip/err.h"

struct netif;
struct pbuf;
struct rte_ring;
extern "C" void rte_ring_free(struct rte_ring*);

namespace openperf::packetio::dpdk::netif_rx_strategy {

class queueing
{
    struct rte_ring_deleter
    {
        void operator()(rte_ring* ring) { rte_ring_free(ring); }
    };

    std::unique_ptr<rte_ring, rte_ring_deleter> m_queue;
    std::atomic_flag m_notify;
    static constexpr int rx_queue_size = 512;

public:
    queueing(std::string_view if_prefix, uint16_t if_index,
             uint16_t port_index);

    err_t handle_rx(pbuf*, netif*);
    err_t handle_rx_notify(netif*);
};

struct direct
{
    err_t handle_rx(pbuf*, netif*);
    err_t handle_rx_notify(netif*);
};

} // namespace openperf::packetio::dpdk::netif_rx_strategy

#endif /* _OP_PACKETIO_DPDK_NET_INTERFACE_RX_HPP_ */
