#include <cerrno>
#include <ctime>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

#include <sys/epoll.h>
#include <sys/eventfd.h>

#include "packetio/drivers/dpdk/model/physical_port.hpp"
#include "packetio/workers/dpdk/tx_queue.hpp"
#include "core/op_log.h"

namespace openperf::packetio::dpdk {

static std::string get_ring_name(uint16_t port_id, uint16_t queue_id)
{
    return (std::string("tx_ring_") + std::to_string(port_id) + "_"
            + std::to_string(queue_id));
}

tx_queue::data::data(uint16_t port_id, uint16_t queue_id)
    : read_idx(0)
    , port(port_id)
    , queue(queue_id)
    , fd(eventfd(0, EFD_NONBLOCK))
    , ring(rte_ring_create(get_ring_name(port_id, queue_id).c_str(),
                           tx_queue_size,
                           rte_eth_dev_socket_id(port_id),
                           RING_F_SP_ENQ | RING_F_SC_DEQ))
    , write_idx(0)
{}

tx_queue::tx_queue(uint16_t port_id, uint16_t queue_id)
    : m_data(port_id, queue_id)
{
    static_assert(offsetof(data, write_idx) - offsetof(data, read_idx) >= 64);
    if (m_data.fd == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }

    if (!m_data.ring) {
        throw std::runtime_error("Could not create DPDK ring for tx queue");
    }
}

tx_queue::~tx_queue() { close(m_data.fd); }

int tx_queue::event_fd() const { return (m_data.fd); }

uint16_t tx_queue::port_id() const { return (m_data.port); }

uint16_t tx_queue::queue_id() const { return (m_data.queue); }

rte_ring* tx_queue::ring() const { return (m_data.ring.get()); }

uint64_t tx_queue::notifications() const
{
    return (m_data.write_idx.load(std::memory_order_acquire)
            - m_data.read_idx.load(std::memory_order_acquire));
}

bool tx_queue::ack()
{
    if (notifications()) {
        uint64_t counter = 0;
        if (eventfd_read(event_fd(), &counter) == 0) {
            m_data.read_idx.fetch_add(counter, std::memory_order_release);
        }
    }
    return (true);
}

bool tx_queue::notify()
{
    if (!notifications()) {
        m_data.write_idx.fetch_add(1, std::memory_order_release);
        if (auto error = eventfd_write(event_fd(), 1U); error != 0) {
            m_data.write_idx.fetch_sub(1, std::memory_order_release);
            OP_LOG(OP_LOG_ERROR,
                   "Could not generate notification for tx "
                   "queue %u:%u on fd %d: %s\n",
                   port_id(),
                   queue_id(),
                   event_fd(),
                   strerror(errno));
            return (false);
        }
    }
    return (true);
}

bool tx_queue::enable()
{
    /*
     * Acknowledge any outstanding data and if the ring isn't empty, create
     * a new notification.
     */
    return (ack() && (rte_ring_empty(ring()) || notify()));
}

void tx_queue::dump() const
{
    fprintf(stderr,
            "write_idx = %zu, read_index = %zu\n",
            m_data.write_idx.load(std::memory_order_acquire),
            m_data.read_idx.load(std::memory_order_acquire));
}

uint16_t tx_queue::enqueue(rte_mbuf* const mbufs[], uint16_t nb_mbufs)
{
    notify();

    auto queued = rte_ring_enqueue_burst(
        ring(), reinterpret_cast<void* const*>(mbufs), nb_mbufs, nullptr);
    if (!queued) {
        OP_LOG(OP_LOG_WARNING, "tx queue %u:%u full\n", port_id(), queue_id());
        return (0);
    };

    return (queued);
}

} // namespace openperf::packetio::dpdk
