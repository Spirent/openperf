#ifndef _ICP_PACKETIO_DPDK_RX_QUEUE_H_
#define _ICP_PACKETIO_DPDK_RX_QUEUE_H_

#include <cstdint>

namespace icp {
namespace packetio {
namespace dpdk {

class rx_queue {
    uint16_t m_port;
    uint16_t m_queue;

public:
    rx_queue(uint16_t port_id, uint16_t queue_id);
    ~rx_queue() = default;

    uint16_t port_id() const;
    uint16_t queue_id() const;
    uint32_t poll_id() const;

    bool add(int poll_fd, void* data);
    bool del(int poll_fd, void* data);

    bool enable();
    bool disable();
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_RX_QUEUE_H_ */
