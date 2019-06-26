#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk {

rx_queue::rx_queue(uint16_t port_id, uint16_t queue_id)
    : m_port(port_id)
    , m_queue(queue_id)
{}

uint16_t rx_queue::port_id() const { return (m_port); }

uint16_t rx_queue::queue_id() const { return (m_queue); }

uint32_t rx_queue::poll_id() const
{
    return ((static_cast<uint32_t>(port_id()) << 16) | queue_id());
}

bool rx_queue::add(int poll_fd, void* data)
{
    auto error = rte_eth_dev_rx_intr_ctl_q(port_id(), queue_id(), poll_fd,
                                           RTE_INTR_EVENT_ADD, data);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not add interrupt event for rx port queue %u:%u: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }
    return (!error);
}

bool rx_queue::del(int poll_fd, void* data)
{
    auto error = rte_eth_dev_rx_intr_ctl_q(port_id(), queue_id(), poll_fd,
                                           RTE_INTR_EVENT_DEL, data);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not delete interrupt event for rx port queue %u:%u: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }
    return (!error);
}

bool rx_queue::enable()
{
    int error = rte_eth_dev_rx_intr_enable(port_id(), queue_id());
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not enable interrupt for rx port queue %d:%d: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }
    return (!error);
}

bool rx_queue::disable()
{
    int error = rte_eth_dev_rx_intr_disable(port_id(), queue_id());
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not disable interrupt for rx port queue %d:%d: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }
    return (!error);
}

}
