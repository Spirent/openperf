#include <array>

#include <sys/epoll.h>

#include "core/icp_core.h"
#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/drivers/dpdk/queue_poller.h"

namespace icp {
namespace packetio {
namespace dpdk {

bool queue_poller::add(uint16_t port_id, uint16_t queue_id)
{
    uintptr_t data = (port_id << 16) | queue_id;
    int error = rte_eth_dev_rx_intr_ctl_q(port_id, queue_id,
                                          RTE_EPOLL_PER_THREAD,
                                          RTE_INTR_EVENT_ADD,
                                          reinterpret_cast<void*>(data));

    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not add interrupt event for port queue %d:%d: %s\n",
                port_id, queue_id, strerror(std::abs(error)));
        return (false);
    }

    m_queues.emplace_back(port_id, queue_id);
    return (true);
}

bool queue_poller::del(uint16_t port_id, uint16_t queue_id)
{
    uintptr_t data = (port_id << 16) | queue_id;
    int error = rte_eth_dev_rx_intr_ctl_q(port_id, queue_id,
                                          RTE_EPOLL_PER_THREAD,
                                          RTE_INTR_EVENT_DEL,
                                          reinterpret_cast<void*>(data));

    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not delete interrupt event for port queue %d:%d %s\n",
                port_id, queue_id, strerror(std::abs(error)));
        return (false);
    }

    m_queues.erase(std::remove(std::begin(m_queues), std::end(m_queues),
                               std::make_pair(port_id, queue_id)), std::end(m_queues));
    return (true);
}

bool queue_poller::enable(uint16_t port_id, uint16_t queue_id)
{
    int error = rte_eth_dev_rx_intr_enable(port_id, queue_id);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not enable interrupt for port queue %d:%d: %s\n",
                port_id, queue_id, strerror(std::abs(error)));
    }
    return (!error);
}

bool queue_poller::disable(uint16_t port_id, uint16_t queue_id)
{
    int error = rte_eth_dev_rx_intr_disable(port_id, queue_id);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not disable interrupt for port queue %d:%d: %s\n",
                port_id, queue_id, strerror(std::abs(error)));
    }
    return (!error);
}


std::vector<std::pair<uint16_t, uint16_t>> queue_poller::poll(int timeout_ms)
{
    std::vector<std::pair<uint16_t, uint16_t>> to_return;

    std::array<struct rte_epoll_event, max_queues> events;
    int n = rte_epoll_wait(RTE_EPOLL_PER_THREAD, events.data(), max_queues, timeout_ms);
    for (int i = 0; i < n; i++) {
        auto data = reinterpret_cast<uintptr_t>(events[i].epdata.data);
        uint16_t port_id = data >> 16;
        uint16_t queue_id = data & std::numeric_limits<uint16_t>::max();
        disable(port_id, queue_id);
        to_return.emplace_back(port_id, queue_id);
    }

    return (to_return);
}

}
}
}
