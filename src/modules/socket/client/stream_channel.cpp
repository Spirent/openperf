#include <cassert>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <numeric>
#include <sys/eventfd.h>
#include <sys/poll.h>

#include <fcntl.h>

#include "socket/client/stream_channel.h"

namespace icp {
namespace socket {
namespace client {

stream_channel::stream_channel(int client_fd, int server_fd)
    : sendq(circular_buffer::client())
    , recvq(circular_buffer::server())
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

stream_channel::~stream_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);
}

static int do_read(int fd)
{
    uint64_t counter;
    return (eventfd_read(fd, &counter));
}

static int do_write(int fd)
{
    return (eventfd_write(fd, 1UL));
}

int stream_channel::client_fd()
{
    return (client_fds.client_fd);
}

int stream_channel::server_fd()
{
    return (client_fds.server_fd);
}

int stream_channel::flags()
{
    return (socket_flags.load(std::memory_order_acquire));
}

int stream_channel::flags(int flags)
{
    socket_flags.store(flags, std::memory_order_release);
    return (0);
}

void stream_channel::clear_event_flags()
{
    block_event_flag.clear(std::memory_order_release);
    notify_event_flag.clear(std::memory_order_release);
}

tl::expected<size_t, int> stream_channel::send(pid_t pid,
                                               const iovec iov[], size_t iovcnt,
                                               const sockaddr *to __attribute__((unused)))
{
    if (auto error = socket_error.load(std::memory_order_relaxed); error != 0) {
        return (tl::make_unexpected(errno));
    }

    size_t buf_available = 0;
    while ((buf_available = sendq.writable()) == 0) {
        /* no writing space available; can we block? */
        auto flags = socket_flags.load(std::memory_order_acquire);
        if (flags & O_NONBLOCK) {
            /*
             * We have a non-blocking eventfd; let's try to stop up the pipe so
             * the client doesn't spin.
             */
            if (!block_event_flag.test_and_set(std::memory_order_acquire)) {
                if (notify_event_flag.test_and_set(std::memory_order_acquire)) {
                    /* consume the existing notification */
                    do_read(client_fd());
                }
                /* no current notifications and no blocks; stuff up the pipe */
                if (eventfd_write(client_fd(), eventfd_max) == -1) {
                    clear_event_flags();
                    return (tl::make_unexpected(errno));
                }
            }
            return (tl::make_unexpected(EWOULDBLOCK));
        }

        assert((flags & O_NONBLOCK) == 0);

        /* yes we can; so block on our eventfd */
        block_event_flag.test_and_set(std::memory_order_acquire);
        notify_event_flag.test_and_set(std::memory_order_acquire);
        if (eventfd_write(client_fd(), eventfd_max) == -1) {
            clear_event_flags();
            return (tl::make_unexpected(errno));
        }

        /* Check if an error woke us up */
        if (auto error = socket_error.load(std::memory_order_acquire); error != 0) {
            return (tl::make_unexpected(error));
        }
    }

    assert(buf_available);

    bool empty = sendq.empty();
    auto written = sendq.write(pid, iov, iovcnt);
    if (written && empty) {
        do_write(server_fd());
    }

    return (written);
}

tl::expected<size_t, int> stream_channel::recv(pid_t pid __attribute__((unused)),
                                               iovec iov[], size_t iovcnt,
                                               sockaddr *from __attribute__((unused)),
                                               socklen_t *fromlen __attribute__((unused)))
{
    if (auto error = socket_error.load(std::memory_order_acquire); error != 0) {
        if (error == EOF) return (0);
        return (tl::make_unexpected(error));
    }

    size_t buf_readable = 0;
    while ((buf_readable = recvq.readable()) == 0) {
        auto result = do_read(client_fd());
        if (result == -1) return (tl::make_unexpected(errno));

        /* we read something; clear all flags */
        clear_event_flags();

        /* check if an error woke us up */
        if (auto error = socket_error.load(std::memory_order_acquire); error != 0) {
            if (error == EOF) return (0);
            return (tl::make_unexpected(error));
        }
    }

    assert(buf_readable);

    auto read = recvq.read(iov, iovcnt);
    if (read) do_write(server_fd());

    /* Check status of available data and update flags/fds */
    if (read < buf_readable) {
        if (!notify_event_flag.test_and_set(std::memory_order_acquire)) {
            /* more to read but no notification; create one */
            do_write(client_fd());
        }
    } else if (notify_event_flag.test_and_set(std::memory_order_acquire)) {
        /* nothing to read and notification exists; consume it */
        do_read(client_fd());
        clear_event_flags();
    } else {
        /*
         * we just set the notification flag in our check above but there's
         * nothing to read, so clear the flag
         */
        notify_event_flag.clear(std::memory_order_release);
    }

    return (read);
}

int stream_channel::recv_clear()
{
    return (do_read(client_fd()));
}

}
}
}
