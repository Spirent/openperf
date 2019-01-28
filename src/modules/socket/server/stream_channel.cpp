#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <numeric>
#include <poll.h>
#include <sys/eventfd.h>

#include "socket/server/stream_channel.h"
#include "lwip/pbuf.h"
#include "core/icp_log.h"

namespace icp {
namespace socket {
namespace server {

stream_channel::stream_channel(int flags)
    : sendq(circular_buffer::server())
    , recvq(circular_buffer::client())
    , notify_event_flag(false)
    , block_event_flag(false)
    , socket_error(0)
    , socket_flags(0)
{
    /* make sure we can use these interchangeably */
    static_assert(O_NONBLOCK == EFD_NONBLOCK);
    static_assert(O_CLOEXEC == EFD_CLOEXEC);

    int event_flags = 0;
    if (flags & SOCK_CLOEXEC)  event_flags |= EFD_CLOEXEC;
    if (flags & SOCK_NONBLOCK) event_flags |= EFD_NONBLOCK;

    if ((server_fds.client_fd = eventfd(0, event_flags)) == -1
        || (server_fds.server_fd = eventfd(0, 0)) == -1) {
        throw std::runtime_error("Could not create eventfd: "
                                 + std::string(strerror(errno)));
    }

    socket_flags.store(event_flags, std::memory_order_release);
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

void stream_channel::maybe_notify()
{
    auto fd = client_fd();

    maybe_unblock();

    if (!notify_event_flag.test_and_set(std::memory_order_acquire)) {
        do_write(fd);
    }

    /* notify flag is set */
}

void stream_channel::maybe_unblock()
{
    auto fd = client_fd();

    if (block_event_flag.test_and_set(std::memory_order_acquire)) {
        do_read(fd);
        notify_event_flag.clear(std::memory_order_release);
    }
    block_event_flag.clear(std::memory_order_release);

    /* block flag is unset */
}

void stream_channel::ack()
{
    do_read(server_fd());
}

void stream_channel::error(int error)
{
    socket_error.store(error, std::memory_order_relaxed);
    maybe_notify();
}

size_t stream_channel::send(pid_t pid, const iovec iov[], size_t iovcnt)
{
    auto written = recvq.write(pid, iov, iovcnt);
    if (!written) {
        ICP_LOG(ICP_LOG_ERROR, "Could not write to client receive socket: %s\n",
                strerror(written.error()));
        return (0);
    }

    maybe_notify();

    return (*written);
}

iovec stream_channel::recv_peek()
{
    return (iovec{ .iov_base = sendq.peek(),
                   .iov_len = sendq.readable() });
}

size_t stream_channel::recv_clear(size_t length)
{
    auto adjust = sendq.skip(length);
    assert(adjust == length);  /* should always be true for us */

    maybe_unblock();

    return (length);
}

}
}
}
