#ifndef _ICP_PACKETIO_DPDK_RX_QUEUE_H_
#define _ICP_PACKETIO_DPDK_RX_QUEUE_H_

#include <cstdint>
#include "packetio/drivers/dpdk/dpdk.h"

namespace icp {
namespace packetio {
namespace dpdk {

class rx_queue {
    uint16_t m_port;
    uint16_t m_queue;
    struct rte_epoll_event m_event;

public:
    rx_queue(uint16_t port_id, uint16_t queue_id);
    ~rx_queue() = default;

    rx_queue(rx_queue&&) = delete;
    rx_queue& operator=(rx_queue&&) = delete;

    uint16_t port_id() const;
    uint16_t queue_id() const;

    bool add(int poll_fd, void* data);
    bool del(int poll_fd, void* data);

    bool enable();
    bool disable();
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_RX_QUEUE_H_ */
