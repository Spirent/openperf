#ifndef _ICP_SOCKET_CLIENT_IO_CHANNEL_H_
#define _ICP_SOCKET_CLIENT_IO_CHANNEL_H_

#include "socket/io_channel.h"

namespace icp {
namespace socket {
namespace client {

class io_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    IO_CHANNEL_MEMBERS

public:
    io_channel(int client_fd, int server_fd);
    ~io_channel();

    io_channel(const io_channel&) = delete;
    io_channel& operator=(const io_channel&&) = delete;

    io_channel(io_channel&&) = default;
    io_channel& operator=(io_channel&&) = default;

    bool send(const iovec& iov);
    void send_wait();

    std::optional<iovec> recv();
    void recv_clear();
};

}
}
}

#endif /* _ICP_SOCKET_CLIENT_IO_CHANNEL_H_ */
