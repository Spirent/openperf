#ifndef _ICP_PACKETIO_DPDK_RX_QUEUE_H_
#define _ICP_PACKETIO_DPDK_RX_QUEUE_H_

#include <atomic>
#include <cstdint>

#include "utils/enum_flags.h"
#include "packetio/drivers/dpdk/dpdk.h"

namespace icp::packetio::dpdk {

enum class rx_feature_flags {
    hardware_lro  = (1 << 0),  /**< Hardware supports LRO */
    hardware_tags = (1 << 1),  /**< Hardware is tagging stack packets */
};

class rx_queue {
public:
    rx_queue(uint16_t port_id, uint16_t queue_id);
    ~rx_queue() = default;

    rx_queue(rx_queue&&) = delete;
    rx_queue& operator=(rx_queue&&) = delete;

    uint16_t port_id() const;
    uint16_t queue_id() const;

    using bitflags = icp::utils::bit_flags<rx_feature_flags>;
    bitflags flags() const;
    void flags(bitflags flags);

    bool add(int poll_fd, void* data);
    bool del(int poll_fd, void* data);

    bool enable();
    bool disable();

private:
    uint16_t m_port;
    uint16_t m_queue;
    std::atomic<bitflags> m_flags;
    struct rte_epoll_event m_event;
};

}

declare_enum_flags(icp::packetio::dpdk::rx_feature_flags);

#endif /* _ICP_PACKETIO_DPDK_RX_QUEUE_H_ */
