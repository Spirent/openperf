#include <cerrno>
#include <numeric>
#include <poll.h>
#include <sys/eventfd.h>

#include "socket/server/stream_channel.h"
#include "lwip/pbuf.h"
#include "core/icp_log.h"

namespace icp {
namespace socket {
namespace server {

stream_channel::stream_channel(int socket_flags)
    : sendq(circular_buffer::server())
    , recvq(circular_buffer::client())
    , socket_error(0)
{
    int event_flags = 0;
    if (socket_flags & SOCK_CLOEXEC)  event_flags |= EFD_CLOEXEC;
    if (socket_flags & SOCK_NONBLOCK) event_flags |= EFD_NONBLOCK;

    if ((server_fds.client_fd = eventfd(0, event_flags)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }
}

stream_channel::~stream_channel()
{
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

static void do_read(int fd)
{
    uint64_t counter;
    eventfd_read(fd, &counter);
}

static void do_write(int fd)
{
    eventfd_write(fd, 1);
}

static bool is_readable(int fd)
{
    struct pollfd to_poll = { .fd = fd,
                              .events = POLLIN };
    auto n = poll(&to_poll, 1, 0);
    return (n == 1
            ? to_poll.revents & POLLIN
            : false);
}

static bool is_writable(int fd)
{
    struct pollfd to_poll = { .fd = fd,
                              .events = POLLOUT };
    auto n = poll(&to_poll, 1, 0);
    return (n == 1
            ? to_poll.revents & POLLOUT
            : false);
}

static void unblock(int fd)
{
    if (is_readable(fd)) {
        do_read(fd);
    }
}

void stream_channel::notify()
{
    auto fd = client_fd();
    if (!is_writable(fd)) {
        do_read(fd);
    }
    do_write(fd);
}

void stream_channel::ack()
{
    do_read(server_fd());
}

void stream_channel::error(int error)
{
    socket_error.store(error, std::memory_order_relaxed);
    if (recvq.empty()) notify();
}

bool stream_channel::send(pid_t pid, const iovec iov[], size_t iovcnt)
{
    auto iov_length = std::accumulate(iov, iov + iovcnt, 0UL,
                                      [](size_t x, const iovec& iov) {
                                          return (x + iov.iov_len);
                                      });

    if (recvq.writable() < iov_length) {
        return (false);
    }

    auto written = recvq.write(pid, iov, iovcnt);
    assert(*written == iov_length);
    notify();

    return (true);
}

size_t stream_channel::send_ack()
{
    return (read_counter.exchange(0, std::memory_order_acq_rel));
}

iovec stream_channel::recv_peek()
{
    return (iovec{ .iov_base = sendq.peek(),
                   .iov_len = sendq.readable() });
}

void stream_channel::recv_skip(size_t length)
{
    auto full = sendq.full();
    auto adjust = sendq.skip(length);
    assert(adjust == length);  /* should always be true for us */
    if (full) unblock(client_fd());
    if (sendq.readable()) do_write(server_fd());  /* not done reading */
}

}
}
}
