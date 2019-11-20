#ifndef _OP_PACKETIO_DPDK_RXTX_QUEUE_CONTAINER_HPP_
#define _OP_PACKETIO_DPDK_RXTX_QUEUE_CONTAINER_HPP_

#include <memory>
#include <vector>

namespace openperf {
namespace packetio {
namespace dpdk {

template <typename RxQueue, typename TxQueue>
class rxtx_queue_container {
    std::vector<std::unique_ptr<RxQueue>> m_rxqs;
    std::vector<std::unique_ptr<TxQueue>> m_txqs;

public:
    rxtx_queue_container(uint16_t port_id, uint16_t nb_rxqs, uint16_t nb_txqs)
    {
        for (uint16_t q = 0; q < nb_rxqs; q++) {
            m_rxqs.push_back(std::make_unique<RxQueue>(port_id, q));
        }

        for (uint16_t q = 0; q < nb_txqs; q++) {
            m_txqs.push_back(std::make_unique<TxQueue>(port_id, q));
        }
    }

    size_t rx_queues() const
    {
        return (m_rxqs.size());
    }

    size_t tx_queues() const
    {
        return (m_txqs.size());
    }

    uint16_t rx_queue_id(uint32_t hash) const
    {
        return (hash % m_rxqs.size());
    }

    uint16_t tx_queue_id(uint32_t hash) const
    {
        return (hash % m_txqs.size());
    }

    RxQueue* rx(uint16_t queue_id) const
    {
        return (m_rxqs.at(queue_id).get());
    }

    RxQueue* rx(uint32_t hash) const
    {
        return (m_rxqs.at(hash % m_rxqs.size()).get());
    }

    TxQueue* tx(uint16_t queue_id) const
    {
        return (m_txqs.at(queue_id).get());
    }

    TxQueue* tx(uint32_t hash) const
    {
        return (m_txqs.at(hash % m_txqs.size()).get());
    }
};

}
}
}

#endif /* _OP_PACKETIO_DPDK_RXTX_QUEUE_CONTAINER_HPP_ */
