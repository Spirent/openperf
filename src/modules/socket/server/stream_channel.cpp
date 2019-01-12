#include <cerrno>
#include <sys/eventfd.h>

#include "socket/server/stream_channel.h"
#include "lwip/pbuf.h"
#include "core/icp_log.h"

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

stream_channel::stream_channel()
    : sendq(circular_buffer::server())
{
    if ((server_fds.client_fd = eventfd(0, 0)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }
}

stream_channel::~stream_channel()
{
    clear_pbuf_queue(freeq);
    while (!recvq.empty()) {
        auto buf = recvq.pop();
        pbuf_free(const_cast<pbuf*>(buf->pbuf()));
    }
    close(server_fds.client_fd);
    close(server_fds.server_fd);
}

int stream_channel::client_fd()
{
    return (server_fds.client_fd);
}

int stream_channel::server_fd()
{
    return (server_fds.server_fd);
}

void stream_channel::notify()
{
    eventfd_write(server_fds.client_fd, 1);
}

bool stream_channel::send(const pbuf *p)
{
    assert(p->next == nullptr);  /* let's not drop data */
    bool empty = recvq.empty();
    bool pushed = recvq.push(stream_channel_buf(p, p->payload, p->len));
    if (pushed && empty) notify();
    return (pushed);
}

void stream_channel::send_wait()
{
    eventfd_write(server_fds.client_fd, eventfd_max);
}

iovec stream_channel::recv()
{
    return (iovec{ .iov_base = sendq.peek(),
                   .iov_len = sendq.readable() });
}

void stream_channel::recv_ack()
{
    uint64_t counter;
    eventfd_read(server_fds.server_fd, &counter);
}

void stream_channel::ack(size_t length)
{
    auto adjust = sendq.skip(length);
    assert(adjust == length);  /* should always be true for us */
}

}
}
}
