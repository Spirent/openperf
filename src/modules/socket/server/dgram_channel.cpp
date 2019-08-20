#include <cassert>
#include <cerrno>
#include <unistd.h>
#include <sys/uio.h>

#include "socket/event_queue_consumer.tcc"
#include "socket/event_queue_producer.tcc"
#include "socket/server/dgram_channel.h"
#include "lwip/pbuf.h"

namespace icp {
namespace socket {

template class event_queue_consumer<server::dgram_channel>;
template class event_queue_producer<server::dgram_channel>;

namespace server {

static constexpr uint16_t max_dgram_length = 1472;  /* 1500 - IP header - UDP header */

/***
 * protected members required for template derived functionality
 ***/
/*
 * We consume notifications on the server fd.
 * We produce notifications on the client fd.
 */
int dgram_channel::consumer_fd() const
{
    return (server_fds.server_fd);
}

int dgram_channel::producer_fd() const
{
    return (server_fds.client_fd);
}

/* We generate notifications for the receive queue */
std::atomic_uint64_t& dgram_channel::notify_read_idx()
{
    return (rx_fd_read_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_read_idx() const
{
    return (rx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::notify_write_idx()
{
    return (rx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::notify_write_idx() const
{
    return (rx_fd_write_idx);
}

/* We generate acknowledgements for the transmit queue */
std::atomic_uint64_t& dgram_channel::ack_read_idx()
{
    return (tx_fd_read_idx);
}

const std::atomic_uint64_t& dgram_channel::ack_read_idx() const
{
    return (tx_fd_read_idx);
}

std::atomic_uint64_t& dgram_channel::ack_write_idx()
{
    return (tx_fd_write_idx);
}

const std::atomic_uint64_t& dgram_channel::ack_write_idx() const
{
    return (tx_fd_write_idx);
}

/***
 * Public members
 ***/

template <typename BipartiteRing>
size_t free_spent_pbufs(BipartiteRing& ring)
{
    std::array<dgram_channel_item, api::socket_queue_length> items;
    auto nb_items = ring.dequeue(items.data(), items.size());
    for (size_t idx = 0; idx < nb_items; idx++) {
        pbuf_free(items[idx].pvec.pbuf());
    }
    return (nb_items);
}

template <typename BipartiteRing>
size_t load_fresh_pbufs(BipartiteRing& ring)
{
    std::array<dgram_channel_item, api::socket_queue_length> items;
    auto capacity = ring.capacity();
    if (!capacity) return (0);
    size_t idx = 0;
    while (pbuf* p = pbuf_alloc(PBUF_TRANSPORT, max_dgram_length, PBUF_POOL)) {
        items[idx++] = dgram_channel_item{ .address = std::nullopt,
                                           .pvec = pbuf_vec(p, p->payload, p->len)};
        if (idx == capacity) break;
    }
    auto enqueued = ring.enqueue(items.data(), capacity);
    assert(enqueued == capacity);
    return (enqueued);
}

template <typename BipartiteRing>
void unload_and_free_all_pbufs(BipartiteRing& ring)
{
    std::array<dgram_channel_item, api::socket_queue_length> items;
    while (ring.available()) {
        ring.unpack();
    }
    ring.repack();
    free_spent_pbufs(ring);
}

dgram_channel::dgram_channel(int socket_flags)
    : sendq(dgram_ring::server())
    , tx_fd_write_idx(0)
    , rx_fd_read_idx(0)
    , recvq(dgram_ring::server())
    , tx_fd_read_idx(0)
    , rx_fd_write_idx(0)
    , socket_flags(0)
{
    int event_flags = 0;
    if (socket_flags & SOCK_CLOEXEC)  event_flags |= EFD_CLOEXEC;
    if (socket_flags & SOCK_NONBLOCK) event_flags |= EFD_NONBLOCK;

    if ((server_fds.client_fd = eventfd(0, event_flags)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }

    load_fresh_pbufs(sendq);
}

dgram_channel::~dgram_channel()
{
    unload_and_free_all_pbufs(recvq);
    unload_and_free_all_pbufs(sendq);
    close(server_fds.client_fd);
    close(server_fds.server_fd);
}

int dgram_channel::client_fd()
{
    return (server_fds.client_fd);
}

int dgram_channel::server_fd()
{
    return (server_fds.server_fd);
}

bool dgram_channel::send_empty() const
{
    return (recvq.empty());
}

/* Push all pbufs in the chain onto the dgram_channel */
bool dgram_channel::send(const pbuf *p)
{
    assert(p->next == nullptr);  /* let's not drop data */
    free_spent_pbufs(recvq);
    bool pushed = recvq.enqueue(
        dgram_channel_item{ .address = std::nullopt,
                            .pvec = pbuf_vec(const_cast<pbuf*>(p),
                                             p->payload, p->len) });
    if (pushed) notify();
    return (pushed);
}

bool dgram_channel::send(const pbuf *p, const dgram_ip_addr* addr, in_port_t port)
{
    assert(p->next == nullptr);  /* scream if we drop data */
    free_spent_pbufs(recvq);
    bool pushed = recvq.enqueue(
        dgram_channel_item{ .address = dgram_channel_addr(addr, port),
                            .pvec = pbuf_vec(const_cast<pbuf*>(p),
                                             p->payload, p->len) });
    if (pushed) notify();
    return (pushed);
}

size_t dgram_channel::recv(dgram_channel_item items[], size_t max_items)
{
    size_t to_dequeue = std::min(sendq.ready(), max_items);
    if (!to_dequeue) return (0);
    auto dequeued = sendq.dequeue(items, to_dequeue);
    /*
     * The client can't adjust the pbuf length from it's process space,
     * so we need to do it here.  Despite the name, 'pbuf_realloc' won't
     * actually allocate anything here; it just trims the length.
     */
    for (size_t idx = 0; idx < dequeued; idx++) {
        auto& [address, pvec] = items[idx];
        pbuf_realloc(pvec.pbuf(), pvec.len());
    }
    load_fresh_pbufs(sendq);
    return (dequeued);
}

}
}
}
