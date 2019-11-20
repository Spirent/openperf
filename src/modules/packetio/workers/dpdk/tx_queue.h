#ifndef _OP_PACKETIO_DPDK_TX_QUEUE_H_
#define _OP_PACKETIO_DPDK_TX_QUEUE_H_

#include <atomic>
#include <memory>

#include "packetio/workers/dpdk/pollable_event.tcc"

struct rte_ring;
extern void rte_ring_free(rte_ring*);

namespace openperf::packetio::dpdk {

class tx_queue : public pollable_event<tx_queue> {
    struct rte_ring_deleter {
        void operator()(struct rte_ring* ring) {
            rte_ring_free(ring);
        }
    };

    /*
     * Use a struct to get the proper alignment between read_idx and write_idx.
     * We want them to be 64 bytes apart, e.g. 1 cache-line.
     */
    struct data {
        std::atomic_uint64_t read_idx;
        uint16_t port;
        uint16_t queue;
        int fd;
        std::unique_ptr<rte_ring, rte_ring_deleter> ring;
        uint64_t pad[5];
        std::atomic_uint64_t write_idx;

        data(uint16_t port, uint16_t queue);
    };

    data m_data;

    /* XXX: calculate a value based on port speed and queue count? */
    static constexpr uint16_t tx_queue_size = 256;

    uint64_t notifications() const;
    bool notify();
    bool ack();

public:
    tx_queue(uint16_t port_id, uint16_t queue_id);
    ~tx_queue();

    int event_fd() const;

    uint16_t port_id() const;
    uint16_t queue_id() const;
    rte_ring* ring() const;

    bool enable();

    void dump() const;
    uint16_t enqueue(rte_mbuf* const mbufs[], uint16_t nb_mbufs);
};


}

#endif /* _OP_PACKETIO_DPDK_TX_QUEUE_H_ */
