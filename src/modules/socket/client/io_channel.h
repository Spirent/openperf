#ifndef _ICP_SOCKET_CLIENT_IO_CHANNEL_H_
#define _ICP_SOCKET_CLIENT_IO_CHANNEL_H_

#include <array>
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

    int send(const iovec iov[], size_t iovcnt, const sockaddr *to);

    struct recv_reply {
        size_t nb_items;
        std::optional<sockaddr_storage> from;
        socklen_t fromlen;
    };
    recv_reply recv(std::array<iovec, api::socket_queue_length>&);
    int recv_clear();
};

}
}
}

#endif /* _ICP_SOCKET_CLIENT_IO_CHANNEL_H_ */
