#ifndef _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_
#define _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_

#include <cstdint>

#include "packetio/drivers/dpdk/queue_utils.hpp"
#include "packetio/workers/dpdk/rxtx_queue_container.hpp"
#include "packetio/workers/dpdk/rx_queue.hpp"
#include "packetio/workers/dpdk/tx_queue.hpp"

struct rte_mbuf;

namespace openperf::packetio::dpdk::worker {

template <typename T>
class singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator= (const singleton) = delete;
protected:
    singleton() {};
};

class rxtx_queues : public singleton<rxtx_queues>
{
public:
    void setup_queues(const std::vector<queue::descriptor>& descriptors);
    void unset_queues();

    rxtx_queue_container<rx_queue, tx_queue>& operator[](int idx);

private:
    std::unordered_map<int, rxtx_queue_container<rx_queue, tx_queue>> m_port_queues;
};

using tx_function = uint16_t (*)(int idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs);

uint16_t tx_copy_function(int idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs);
uint16_t tx_direct_function(int idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs);
uint16_t tx_dummy_function(int idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs);
uint16_t tx_queue_function(int idx, uint32_t hash, struct rte_mbuf* mbufs[], uint16_t nb_mbufs);

}

#endif /* _OP_PACKETIO_DPDK_WORKER_QUEUES_HPP_ */
