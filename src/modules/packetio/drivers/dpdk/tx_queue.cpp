#include <cerrno>
#include <chrono>
#include <memory>
#include <string>
#include <time.h>
#include <unistd.h>
#include <vector>

#include <sys/epoll.h>
#include <sys/timerfd.h>

#include "packetio/drivers/dpdk/model/physical_port.h"
#include "packetio/drivers/dpdk/tx_queue.h"
#include "core/icp_log.h"

namespace icp {
namespace packetio {
namespace dpdk {

using namespace std::chrono_literals;
static constexpr auto tcp_transmit_delay = 50us;

static std::string get_ring_name(uint16_t port_id, uint16_t queue_id)
{
    return (std::string("tx_ring_") + std::to_string(port_id)
            + "_" + std::to_string(queue_id));
}

/* Read the fd to clear it */
static void tx_queue_epoll_callback(int fd,
                                    void* arg __attribute__((unused)))
{
    uint64_t counter = 0;
    while (read(fd, &counter, sizeof(counter)) < 0) {
        if (errno == EINTR) continue;
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            ICP_LOG(ICP_LOG_ERROR, "Error reading from timer fd %d: %s\n",
                    fd, strerror(errno));
        }
        break;
    }
}

tx_queue::tx_queue(uint16_t port_id, uint16_t queue_id)
    : m_port(port_id)
    , m_queue(queue_id)
    , m_fd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK))
    , m_enabled(false)
    , m_armed(false)
    , m_flushed(false)
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
            .cb_fun = tx_queue_epoll_callback,
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
        auto disarm = itimerspec{
            .it_interval = {0, 0},
            .it_value = {0, 0}
        };
        if (timerfd_settime(m_fd, 0, &disarm, nullptr) == -1) {
            ICP_LOG(ICP_LOG_ERROR, "Could not disarm timerfd for tx queue on %u:%u: %s\n",
                    port_id(), queue_id(), strerror(errno));
            return (false);
        }
    }
    m_enabled.store(false, std::memory_order_release);
    m_flushed.clear(std::memory_order_release);
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
            /*
             * If we haven't armed the timer, do so.  We need to let the tx worker
             * know it has packets to send.  The small delay is to hopefully allow
             * the worker to send a burst of packets.
             */
            auto ns_delay = std::chrono::duration_cast<std::chrono::nanoseconds>(tcp_transmit_delay);
            auto alarm = itimerspec{
                .it_interval = { 0, 0 },
                .it_value    = { 0, ns_delay.count() }
            };
            if (timerfd_settime(m_fd, 0, &alarm, nullptr) == -1) {
                ICP_LOG(ICP_LOG_ERROR, "Could not arm timerfd for tx queue on %u:%u: %s\n",
                        port_id(), queue_id(), strerror(errno));
                m_armed.clear(std::memory_order_release);  /* not armed */
            }
        } else if (rte_ring_count(ring()) > (tx_queue_size / 4)
            && !m_flushed.test_and_set(std::memory_order_acquire)) {
            /*
             * If the ring is filling up and we haven't setup a flush timer, do so.
             * We need to get this buffer transmitted before it fills up.
             */
            auto flush = itimerspec{
                .it_interval = { 0, 0},
                .it_value = { 0, 1}
            };
            if (timerfd_settime(m_fd, 0, &flush, nullptr) == -1) {
                ICP_LOG(ICP_LOG_ERROR, "Could not flush timerfd for tx queue on %u:%u: %s\n",
                        port_id(), queue_id(), strerror(errno));
                m_flushed.clear(std::memory_order_release);
            }
        }
    }

    return (queued);
}

}
}
}
