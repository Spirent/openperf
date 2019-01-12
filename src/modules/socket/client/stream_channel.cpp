#include <cerrno>
#include <unistd.h>
#include <numeric>
#include <sys/eventfd.h>

#include "socket/client/stream_channel.h"

namespace icp {
namespace socket {
namespace client {

stream_channel::stream_channel(int client_fd, int server_fd)
    : sendq(circular_buffer::client())
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
    auto buf_available = sendq.writable();
    if (!buf_available) {
        return (tl::make_unexpected(EAGAIN));
    }

    bool empty = (buf_available == sendq.size());
    auto written = sendq.write(pid, iov, iovcnt);
    if (empty) eventfd_write(client_fds.server_fd, 1);
    return (written);
}

tl::expected<size_t, int> stream_channel::recv(pid_t pid, iovec iov[], size_t iovcnt,
                                               sockaddr *from __attribute__((unused)),
                                               socklen_t *fromlen __attribute__((unused)))
{
    std::array<iovec, api::socket_queue_length> items;
    size_t nb_items = 0, item_length = 0;

    auto recv_length = std::accumulate(iov, iov + iovcnt, 0UL,
                                       [](size_t x, const iovec& iov) -> size_t {
                                           return (x + iov.iov_len);
                                       });

    bool full = recvq.full();

    /* handle any partial read, if any */
    if (partial.length()) {
        items[nb_items++] = iovec{
            .iov_base = const_cast<void*>(partial.payload()),
            .iov_len = partial.length()
        };
        item_length += partial.length();
        if (item_length > recv_length) {
            /* ugh... again? */
            auto extra_length = item_length - recv_length;
            auto offset_payload = reinterpret_cast<void*>(
                reinterpret_cast<uintptr_t>(partial.payload()) + partial.length() - extra_length);
            partial = stream_channel_buf(partial.pbuf(), offset_payload, extra_length);
            goto do_process_vm_writev;  /* XXX */
        }
        if (!freeq.push(partial.pbuf())) {
            fprintf(stderr, "Could not return pbuf %p to stack\n",
                    reinterpret_cast<const void*>(partial.pbuf()));
        }
    }

    while (auto item = recvq.pop()) {
        auto &buf = *item;
        items[nb_items++] = iovec{
            .iov_base = const_cast<void*>(buf.payload()),
            .iov_len = buf.length()
        };
        item_length += buf.length();
        if (item_length > recv_length) {
            /*
             * we won't be able to copy all of this iovec out, so
             * stick the remainder in our partial entry.
             */
            auto extra_length = item_length - recv_length;
            auto offset_payload = reinterpret_cast<void*>(
                reinterpret_cast<uintptr_t>(buf.payload()) + buf.length() - extra_length);
            partial = stream_channel_buf(buf.pbuf(), offset_payload, extra_length);
        } else {
            if (!freeq.push(buf.pbuf())) {
                fprintf(stderr, "Could not return pbuf %p to stack\n",
                        reinterpret_cast<const void*>(buf.pbuf()));
            }
        }

        if (nb_items == api::socket_queue_length) break;
    }

    if (nb_items && full) {
        eventfd_write(server_fds.client_fd, 1);
    }

do_process_vm_writev:
    if (item_length < recv_length) {  /* no partial */
        partial = stream_channel_buf();
    }

    auto result = process_vm_readv(pid, iov, iovcnt, items.data(), nb_items, 0);
    if (result == -1) {
        return (tl::make_unexpected(errno));
    }
    return (result);
}

int stream_channel::recv_clear()
{
    uint64_t counter = 0;
    return (eventfd_read(client_fds.client_fd, &counter));
}

}
}
}
