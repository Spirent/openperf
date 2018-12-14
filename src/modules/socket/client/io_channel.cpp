#include <unistd.h>
#include <sys/eventfd.h>

#include "socket/client/io_channel.h"

namespace icp {
namespace socket {
namespace client {

io_channel::io_channel(int client_fd, int server_fd)
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

io_channel::~io_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);
}

bool io_channel::send(const iovec& iov)
{
    bool empty = sendq.empty();
    bool pushed = sendq.push(const_cast<iovec&>(iov));
    if (pushed && empty) {
        eventfd_write(client_fds.server_fd, 1);
    }
    return (pushed);
}

void io_channel::send_wait()
{
    eventfd_write(client_fds.server_fd, eventfd_max);
}

std::optional<iovec> io_channel::recv()
{
    bool full = recvq.full();
    auto item = recvq.pop();
    if (!item) {
        return (std::nullopt);
    }
    if (full) {
        eventfd_write(client_fds.server_fd, 1);
    }
    if (!returnq.push(reinterpret_cast<uintptr_t>(item->pbuf))) {
        printf("Lost pbuf!\n");
    }
    return (std::make_optional<const iovec>({item->ptr, item->size}));
}

void io_channel::recv_clear()
{
    uint64_t counter = 0;
    eventfd_read(client_fds.client_fd, &counter);
}

}
}
}
