#ifndef _ICP_SOCKET_CLIENT_STREAM_CHANNEL_H_
#define _ICP_SOCKET_CLIENT_STREAM_CHANNEL_H_

#include "socket/stream_channel.h"

namespace icp {
namespace socket {
namespace client {

class stream_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    STREAM_CHANNEL_MEMBERS

    int client_fd() const;
    int server_fd() const;

    void clear_event_flags();

public:
    stream_channel(int client_fd, int server_fd);
    ~stream_channel();

    stream_channel(const stream_channel&) = delete;
    stream_channel& operator=(const stream_channel&&) = delete;

    stream_channel(stream_channel&&) = default;
    stream_channel& operator=(stream_channel&&) = default;

    int flags() const;
    int flags(int);

    tl::expected<size_t, int> send(pid_t pid, const iovec iov[], size_t iovcnt,
                                   const sockaddr *to);

    tl::expected<size_t, int> recv(pid_t pid, iovec iov[], size_t iovcnt,
                                   sockaddr *from, socklen_t *fromlen);

    int recv_clear();
};

}
}
}

#endif /* _ICP_SOCKET_CLIENT_STREAM_CHANNEL_H_ */
