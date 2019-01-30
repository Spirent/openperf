#ifndef _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_
#define _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_

#include "socket/api.h"
#include "socket/dgram_channel.h"

namespace icp {
namespace socket {
namespace client {

class dgram_channel
{
    static constexpr uint64_t eventfd_max = std::numeric_limits<uint64_t>::max() - 1;
    DGRAM_CHANNEL_MEMBERS

public:
    dgram_channel(int client_fd, int server_fd);
    ~dgram_channel();

    dgram_channel(const dgram_channel&) = delete;
    dgram_channel& operator=(const dgram_channel&&) = delete;

    dgram_channel(dgram_channel&&) = default;
    dgram_channel& operator=(dgram_channel&&) = default;

    int flags();
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

#endif /* _ICP_SOCKET_CLIENT_DGRAM_CHANNEL_H_ */
