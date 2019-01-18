#include <cassert>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#include "socket/server/dgram_channel.h"
#include "lwip/pbuf.h"

namespace icp {
namespace socket {
namespace server {

static constexpr uint16_t max_dgram_length = 1472;  /* 1500 - IP header - UDP header */

template <typename BipartiteRing>
size_t free_spent_pbufs(BipartiteRing& ring)
{
    std::array<dgram_channel_item, api::socket_queue_length> items;
    auto nb_items = ring.dequeue(items.data(), items.size());
    for (size_t idx = 0; idx < nb_items; idx++) {
        pbuf_free(items[idx].data.pbuf());
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
                                           .data = dgram_channel_data(p, p->payload, p->len)};
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
    : recvq(dgram_ring::server())
    , sendq(dgram_ring::server())
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

void dgram_channel::notify()
{
    eventfd_write(server_fds.client_fd, 1);
}

void dgram_channel::ack()
{
    uint64_t counter = 0;
    eventfd_read(server_fds.server_fd, &counter);

    /* Now is a good time to garbage collect the recvq, too */
    free_spent_pbufs(recvq);
}

/* Push all pbufs in the chain onto the dgram_channel */
bool dgram_channel::send(const pbuf *p)
{
    assert(p->next == nullptr);  /* let's not drop data */
    free_spent_pbufs(recvq);
    bool idle = recvq.idle();
    bool pushed = recvq.enqueue(
        dgram_channel_item{ .address = std::nullopt,
                            .data = dgram_channel_data(const_cast<pbuf*>(p),
                                                       p->payload, p->len) });
    if (pushed && idle) notify();
    return (pushed);
}

bool dgram_channel::send(const pbuf *p, const dgram_ip_addr* addr, in_port_t port)
{
    assert(p->next == nullptr);  /* scream if we drop data */
    free_spent_pbufs(recvq);
    bool idle = recvq.idle();
    bool pushed = recvq.enqueue(
        dgram_channel_item{ .address = dgram_channel_addr(addr, port),
                            .data = dgram_channel_data(const_cast<pbuf*>(p),
                                                       p->payload, p->len) });
    if (pushed && idle) notify();
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
        auto& [address, data] = items[idx];
        pbuf_realloc(data.pbuf(), data.length());
    }
    load_fresh_pbufs(sendq);
    return (dequeued);
}

}
}
}
