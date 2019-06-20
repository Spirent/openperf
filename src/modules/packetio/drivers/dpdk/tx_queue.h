#ifndef _ICP_PACKETIO_DPDK_TX_QUEUE_H_
#define _ICP_PACKETIO_DPDK_TX_QUEUE_H_

#include <atomic>
#include <memory>

//#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/common/dpdk/pollable_event.tcc"

namespace icp::packetio::dpdk {

class tx_queue : public pollable_event<tx_queue> {
    struct rte_ring_deleter {
        void operator()(struct rte_ring* ring) {
            rte_ring_free(ring);
        }
    };

    uint16_t m_port;
    uint16_t m_queue;
    int m_fd;
    std::atomic_bool m_enabled;
    std::atomic_flag m_armed;
    std::unique_ptr<rte_ring, rte_ring_deleter> m_ring;

    /* XXX: calculate a value based on port speed and queue count? */
    static constexpr uint16_t tx_queue_size = 512;

public:
    tx_queue(uint16_t port_id, uint16_t queue_id);
    ~tx_queue();

    int event_fd() const;

    uint16_t port_id() const;
    uint16_t queue_id() const;
    uint32_t poll_id() const;
    rte_ring* ring() const;

    bool enable();
    bool disable();

    uint16_t enqueue(rte_mbuf* const mbufs[], uint16_t nb_mbufs);
};


}

#endif /* _ICP_PACKETIO_DPDK_TX_QUEUE_H_ */
