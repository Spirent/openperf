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

template <typename Queue>
static void clear_pbuf_queue(Queue& queue)
{
    while (auto item = queue.pop()) {
        pbuf_free(const_cast<pbuf*>(
                      reinterpret_cast<const pbuf*>(*item)));
    }
}

dgram_channel::dgram_channel()
{
    if ((server_fds.client_fd = eventfd(0, 0)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }
}

dgram_channel::~dgram_channel()
{
    clear_pbuf_queue(freeq);
    while (!recvq.empty()) {
        auto item = recvq.pop();
        if (std::holds_alternative<dgram_channel_buf>(*item)) {
            auto& buf = std::get<dgram_channel_buf>(*item);
            pbuf_free(const_cast<pbuf*>(buf.pbuf()));
        }
    }
    while (!sendq.empty()) {
        (void)sendq.pop();
    }

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

/* Push all pbufs in the chain onto the dgram_channel */
bool dgram_channel::send(const pbuf *p)
{
    assert(p->next == nullptr);  /* let's not drop data */
    bool empty = recvq.empty();
    bool pushed = recvq.push(dgram_channel_buf(p, p->payload, p->len));
    if (pushed && empty) notify();
    return (pushed);
}

bool dgram_channel::send(const pbuf *p, const dgram_ip_addr* addr, in_port_t port)
{
    assert(p->next == nullptr);  /* scream if we drop data */
    bool empty = recvq.empty();
    bool pushed = recvq.push(dgram_channel_buf(p, p->payload, p->len, true));
    if (!pushed) return (false);
    pushed |= recvq.push(dgram_channel_addr(addr, port));
    if (!pushed) {
        recvq.pop();
        return (false);
    }
    if (empty) notify();
    return (true);
}

void dgram_channel::send_wait()
{
    eventfd_write(server_fds.client_fd, eventfd_max);
}

dgram_channel::recv_reply dgram_channel::recv(std::array<iovec, api::socket_queue_length>& items)
{
    bool full = sendq.full();

    recv_reply to_return = {
        .nb_items = 0,
        .dest = std::nullopt
    };

    while (auto item = sendq.pop()) {
        std::visit(overloaded_visitor(
                       [&](dgram_channel_buf& buf) {
                           items[to_return.nb_items++] = iovec{
                               .iov_base = const_cast<void*>(buf.payload()),
                               .iov_len = buf.length()
                           };
                       },
                       [&](dgram_channel_addr& addr) {
                           to_return.dest = std::make_optional(addr);
                       }),
                   *item);
        if (!more(*item)) break;
    }
    if (to_return.nb_items && full) {
        eventfd_write(server_fds.client_fd, 1);
    }
    return (to_return);
}

void dgram_channel::recv_ack()
{
    uint64_t counter = 0;
    eventfd_read(server_fds.server_fd, &counter);
}

void dgram_channel::garbage_collect()
{
    clear_pbuf_queue(freeq);
}

}
}
}
