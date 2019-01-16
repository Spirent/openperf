#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <numeric>
#include <sys/eventfd.h>

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

tl::expected<size_t, int> stream_channel::send(pid_t pid,
                                               const iovec iov[], size_t iovcnt,
                                               const sockaddr *to __attribute__((unused)))
{
    size_t buf_available = 0;
    while ((buf_available = sendq.writable()) == 0) {
        /* try to block on the fd */
        auto result = eventfd_write(client_fds.client_fd, eventfd_max);
        if (result == -1) return (tl::make_unexpected(errno));
    }

    bool empty = sendq.empty();
    auto written = sendq.write(pid, iov, iovcnt);
    if (written && empty) eventfd_write(client_fds.server_fd, 1);
    return (written);
}

tl::expected<size_t, int> stream_channel::recv(pid_t pid __attribute__((unused)),
                                               iovec iov[], size_t iovcnt,
                                               sockaddr *from __attribute__((unused)),
                                               socklen_t *fromlen __attribute__((unused)))
{
    auto buf_readable = recvq.readable();
    if (!buf_readable) {
        return (0);
    }

    bool full = (recvq.full());
    auto read = recvq.read(iov, iovcnt);
    if (full) eventfd_write(client_fds.server_fd, 1);

    /*
     * We didn't read all available data, so make sure the client knows
     * to come back.
     */
    if (read < buf_readable) eventfd_write(client_fds.client_fd, 1);

    return (read);
}

int stream_channel::recv_clear()
{
    uint64_t counter = 0;
    return (eventfd_read(client_fds.client_fd, &counter));
}

}
}
}
