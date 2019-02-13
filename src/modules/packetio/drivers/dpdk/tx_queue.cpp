#include <cerrno>
#include <memory>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "packetio/drivers/dpdk/model/physical_port.h"
#include "packetio/drivers/dpdk/tx_queue.h"
#include "core/icp_log.h"

namespace icp {
namespace packetio {
namespace dpdk {

static std::string get_ring_name(uint16_t port_id, uint16_t queue_id)
{
    return (std::string("tx_ring_") + std::to_string(port_id)
            + "_" + std::to_string(queue_id));
}

tx_queue::tx_queue(uint16_t port_id, uint16_t queue_id)
    : m_port(port_id)
    , m_queue(queue_id)
    , m_fd(eventfd(0, EFD_NONBLOCK))
    , m_enabled(false)
    , m_armed(false)
    , m_ring(rte_ring_create(get_ring_name(port_id, queue_id).c_str(),
                             tx_queue_size,
                             SOCKET_ID_ANY,
                             RING_F_SP_ENQ | RING_F_SC_DEQ))
{
    if (m_fd == -1) {
        throw std::runtime_error("Could not create timerfd: "
                                 + std::string(strerror(errno)));
    }

    if (!m_ring) {
        throw std::runtime_error("Could not create DPDK ring for tx queue");
    }

}

tx_queue::~tx_queue() { close(m_fd); }

uint16_t tx_queue::port_id() const { return (m_port); }

uint16_t tx_queue::queue_id() const { return (m_queue); }

rte_ring* tx_queue::ring() const { return (m_ring.get()); }

bool tx_queue::add(int poll_fd, void* data)
{
    /*
     * Make sure the event always has the correct data before use.  The
     * DPDK functions will wipe the struct when/if we remove the event.
     */
    m_event = rte_epoll_event{
        .epdata = {
            .event = EPOLLIN | EPOLLET,
            .data = data,
            .cb_fun = nullptr,
            .cb_arg = nullptr
        }
    };

    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_ADD,
                               m_fd, &m_event);
    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not add poll event for tx queue on %u:%u: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }

    return (!error);
}

bool tx_queue::del(int poll_fd, void* data)
{
    assert(m_event.epdata.data == data);
    auto error = rte_epoll_ctl(poll_fd, EPOLL_CTL_DEL,
                               m_fd, &m_event);

    if (error) {
        ICP_LOG(ICP_LOG_ERROR, "Could not delete poll event for tx queue on %u:%u: %s\n",
                port_id(), queue_id(), strerror(std::abs(error)));
    }

    return (!error);
}

bool tx_queue::enable()
{
    m_enabled.store(true, std::memory_order_relaxed);
    return (true);
}

bool tx_queue::disable()
{
    if (m_armed.test_and_set(std::memory_order_acquire)) {
        uint64_t counter = 0;
        if (eventfd_read(m_fd, &counter) == -1) {
            ICP_LOG(ICP_LOG_ERROR, "Could not clear fd %d for tx queue %u:%u: %s\n",
                    m_fd, port_id(), queue_id(), strerror(errno));
            return (false);
        }
    }
    m_enabled.store(false, std::memory_order_release);
    m_armed.clear(std::memory_order_release);
    return (true);
}

uint16_t tx_queue::enqueue(rte_mbuf* const mbufs[], uint16_t nb_mbufs)
{
    auto queued = rte_ring_enqueue_burst(ring(),
                                         reinterpret_cast<void* const*>(mbufs),
                                         nb_mbufs, nullptr);
    if (!queued) {
        ICP_LOG(ICP_LOG_WARNING, "tx queue %u:%u full\n", port_id(), queue_id());
        return (0);
    };

    if (m_enabled.load(std::memory_order_acquire)) {
        if (!m_armed.test_and_set(std::memory_order_acquire)) {
            /* If we haven't armed the eventfd, do so now. */
            if (eventfd_write(m_fd, 1U) == -1) {
                ICP_LOG(ICP_LOG_ERROR, "Could not write notification to fd %d for tx queue %u:%u: %s\n",
                        m_fd, port_id(), queue_id(), strerror(errno));
                m_armed.clear(std::memory_order_release); /* not armed */
            }
        }
    }

    return (queued);
}

}
}
}
