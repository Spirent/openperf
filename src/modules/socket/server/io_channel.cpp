#include <cerrno>
#include <string>
#include <unistd.h>
#include <sys/eventfd.h>

#include "socket/server/io_channel.h"
#include "lwip/pbuf.h"

namespace icp {
namespace socket {
namespace server {

template <typename Queue>
static void clear_pbuf_queue(Queue& queue)
{
    while (auto item = queue.pop()) {
        pbuf_free(reinterpret_cast<pbuf*>(*item));
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
    clear_pbuf_queue(returnq);
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
    bool empty = recvq.empty();
    bool pushed = recvq.push(pbuf_data{
            .pbuf = reinterpret_cast<uintptr_t>(p),
            .ptr = p->payload,
            .size = p->len,
        });
    if (pushed && empty) {
        eventfd_write(server_fds.client_fd, 1);
    }
    return (pushed);
}

void io_channel::send_wait()
{
    clear_pbuf_queue(returnq);
    eventfd_write(server_fds.client_fd, eventfd_max);
}

std::optional<iovec> io_channel::recv()
{
    bool full = sendq.full();
    auto item = sendq.pop();
    if (item && full) {
        eventfd_write(server_fds.client_fd, 1);
    }
    return (item);
}

void io_channel::recv_clear()
{
    clear_pbuf_queue(returnq);
    uint64_t counter = 0;
    eventfd_read(server_fds.server_fd, &counter);
}

}
}
}
