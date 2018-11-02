#ifndef _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_
#define _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_

#include <cstdint>
#include <utility>
#include <vector>

namespace icp {
namespace packetio {
namespace dpdk {

class queue_poller {
    std::vector<std::pair<uint16_t, uint16_t>> m_queues;
    static constexpr size_t max_queues = 128;
public:
    bool add(uint16_t port_id, uint16_t queue_id);
    bool del(uint16_t port_id, uint16_t queue_id);

    bool  enable(uint16_t port_id, uint16_t queue_id);
    bool disable(uint16_t port_id, uint16_t queue_id);

    std::vector<std::pair<uint16_t, uint16_t>> poll(int timeout_ms = -1);
};

}
}
}

#endif /* _ICP_PACKETIO_DPDK_QUEUE_POLLER_H_ */
