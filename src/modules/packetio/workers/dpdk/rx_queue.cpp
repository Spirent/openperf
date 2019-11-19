#include <optional>
#include <sys/epoll.h>

#include "packetio/drivers/dpdk/dpdk.h"
#include "packetio/workers/dpdk/rx_queue.h"
#include "core/icp_log.h"

namespace icp::packetio::dpdk {

static std::optional<int> get_queue_fd(uint16_t port_id, uint16_t queue_id)
{
    /* XXX: clean this up when function symbol is marked stable */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    auto fd = rte_eth_dev_rx_intr_ctl_q_get_fd(port_id, queue_id);
#pragma clang diagnostic pop

    if (fd == -1) return (std::nullopt);
    return (fd);
}

static void ack_interrupt(int fd, void* arg)
{
    auto rxq = reinterpret_cast<rx_queue*>(arg);
    rxq->disable();
}

rx_queue::rx_queue(uint16_t port_id, uint16_t queue_id)
    : m_port(port_id)
    , m_queue(queue_id)
    , m_flags(0)
{}

uint16_t rx_queue::port_id() const { return (m_port); }

uint16_t rx_queue::queue_id() const { return (m_queue); }

rx_queue::bitflags rx_queue::flags() const
{
    return (m_flags.load(std::memory_order_consume));
}

void rx_queue::flags(bitflags flags)
{
    m_flags.store(flags, std::memory_order_release);
}

bool rx_queue::add(int poll_fd, void* data)
{
    auto fd = get_queue_fd(port_id(), queue_id());
    if (!fd) return (false);

    m_event = rte_epoll_event{
        .epdata = {
            .event = EPOLLIN | EPOLLET,
            .data = data,
            .cb_fun = ack_interrupt,
            .cb_arg = this
        }
    };

    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_ADD, *fd, &m_event);

    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not add rx interrupt for %u:%u: %s\n",
                port_id(), queue_id(),strerror(errno));
    }

    return (!error);
}

bool rx_queue::del(int poll_fd, void* data)
{
    auto fd = get_queue_fd(port_id(), queue_id());
    if (!fd) return (false);

    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_DEL, *fd, &m_event);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not delete rx interrupt for %u:%u: %s\n",
                port_id(), queue_id(), strerror(errno));
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
