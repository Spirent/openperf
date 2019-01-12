#include <cerrno>
#include <numeric>
#include <sys/eventfd.h>

#include "socket/server/stream_channel.h"
#include "lwip/pbuf.h"
#include "core/icp_log.h"

namespace icp {
namespace socket {
namespace server {

stream_channel::stream_channel()
    : sendq(circular_buffer::server())
    , recvq(circular_buffer::client())
{
    if ((server_fds.client_fd = eventfd(0, 0)) == -1
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

void stream_channel::notify()
{
    eventfd_write(server_fds.client_fd, 1);
}

void stream_channel::ack()
{
    uint64_t counter;
    eventfd_read(server_fds.server_fd, &counter);
}

bool stream_channel::send(pid_t pid, const iovec iov[], size_t iovcnt)
{
    auto buf_available = recvq.writable();

    auto iov_length = std::accumulate(iov, iov + iovcnt, 0UL,
                                      [](size_t x, const iovec& iov) {
                                          return (x + iov.iov_len);
                                      });

    if (buf_available < iov_length) {
        return (false);
    }

    bool empty = (buf_available == recvq.size());
    auto written = recvq.write(pid, iov, iovcnt);
    if (*written != iov_length) {
        ICP_LOG(ICP_LOG_ERROR, "written = %zu, iov_length = %zu\n",
                *written, iov_length);
    }
    if (empty) notify();
    return (true);
}

void stream_channel::send_wait()
{
    eventfd_write(server_fds.client_fd, eventfd_max);
}

iovec stream_channel::recv_peek()
{
    return (iovec{ .iov_base = sendq.peek(),
                   .iov_len = sendq.readable() });
}

void stream_channel::recv_skip(size_t length)
{
    auto adjust = sendq.skip(length);
    assert(adjust == length);  /* should always be true for us */
}

}
}
}
