#include <cassert>
#include <cerrno>
#include <string>
#include <unistd.h>
#include <sys/eventfd.h>
#include <sys/uio.h>

#include "socket/server/io_channel.h"
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

io_channel::io_channel()
{
    if ((server_fds.client_fd = eventfd(0, 0)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }
}

io_channel::~io_channel()
{
    clear_pbuf_queue(freeq);
    while (!recvq.empty()) {
        (void)recvq.pop();
    }
    while (!sendq.empty()) {
        (void)sendq.pop();
    }

    close(server_fds.client_fd);
    close(server_fds.server_fd);
}

int io_channel::client_fd()
{
    return (server_fds.client_fd);
}

int io_channel::server_fd()
{
    return (server_fds.server_fd);
}

/* Push all pbufs in the chain onto the io_channel */
bool io_channel::send(const pbuf *p)
{
    assert(p->next == nullptr);  /* let's not drop data */
    bool empty = recvq.empty();
    bool pushed = recvq.push(io_channel_buf(p, p->payload, p->len));
    if (pushed && empty) {
        eventfd_write(server_fds.client_fd, 1);
    }
    return (pushed);
}

bool io_channel::send(const pbuf *p, const io_ip_addr* addr, in_port_t port)
{
    assert(p->next == nullptr);  /* scream if we drop data */
    bool empty = recvq.empty();
    bool pushed = recvq.push(io_channel_buf(p, p->payload, p->len, true));
    if (!pushed) return (false);
    pushed |= recvq.push(io_channel_addr(addr, port));
    if (!pushed) {
        recvq.pop();
        return (false);
    }
    if (empty) eventfd_write(server_fds.client_fd, 1);
    return (true);
}

void io_channel::send_wait()
{
    eventfd_write(server_fds.client_fd, eventfd_max);
}

io_channel::recv_reply io_channel::recv(std::array<iovec, api::socket_queue_length>& items)
{
    bool full = sendq.full();

    recv_reply to_return = {
        .nb_items = 0,
        .dest = std::nullopt
    };

    while (auto item = sendq.pop()) {
        std::visit(overloaded_visitor(
                       [&](io_channel_buf& buf) {
                           items[to_return.nb_items++] = iovec{
                               .iov_base = const_cast<void*>(buf.payload()),
                               .iov_len = buf.length()
                           };
                       },
                       [&](io_channel_addr& addr) {
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

void io_channel::recv_ack()
{
    uint64_t counter = 0;
    eventfd_read(server_fds.server_fd, &counter);
}

void io_channel::garbage_collect()
{
    clear_pbuf_queue(freeq);
}

}
}
}
