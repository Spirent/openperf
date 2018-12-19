#ifndef _ICP_SOCKET_API_CLIENT_H_
#define _ICP_SOCKET_API_CLIENT_H_

#include <memory>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unordered_map>

#include "socket/api.h"
#include "socket/client/io_channel.h"
#include "socket/shared_segment.h"
#include "socket/unix_socket.h"
#include "socket/uuid.h"

namespace icp {
namespace socket {
namespace api {

template <typename T>
class thread_singleton {
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    thread_singleton(const thread_singleton&) = delete;
    thread_singleton& operator=(const thread_singleton) = delete;

protected:
    thread_singleton() {};
};

class client : public thread_singleton<client>
{
    uuid m_uuid;
    unix_socket m_sock;

    /* XXX: need a multi-threaded solution in case fd's jump threads */
    struct io_channel_deleter {
        void operator()(socket::client::io_channel *c) {
            c->~io_channel();
        }
    };

    typedef std::unique_ptr<socket::client::io_channel, io_channel_deleter> channel_ptr;

    struct ided_channel {
        socket_id id;
        channel_ptr channel;
    };
    std::unordered_map<int, ided_channel> m_channels;

    std::unique_ptr<memory::shared_segment> m_shm;
    pid_t m_server_pid;

public:
    client();

    void init();

    /* General socket functions */
    int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
    int bind(int s, const struct sockaddr *name, socklen_t namelen);
    int shutdown(int s, int how);
    int getpeername(int s, struct sockaddr *name, socklen_t *namelen);
    int getsockname(int s, struct sockaddr *name, socklen_t *namelen);
    int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
    int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
    int close(int s);
    int connect(int s, const struct sockaddr *name, socklen_t namelen);
    int listen(int s, int backlog);
    int socket(int domain, int type, int protocol);
    int ioctl(int s, long cmd, void *argp);

    /* Receive functions */
    ssize_t read(int s, void *mem, size_t len);
    ssize_t readv(int s, struct iovec *iov, int iovcnt);
    ssize_t recv(int s, void *mem, size_t len, int flags);
    ssize_t recvfrom(int s, void *mem, size_t len, int flags,
                     struct sockaddr *from, socklen_t *fromlen);
    ssize_t recvmsg(int s, struct msghdr *message, int flags);

    /* Transmit functions */
    ssize_t send(int s, const void *dataptr, size_t len, int flags);
    ssize_t sendmsg(int s, const struct msghdr *message, int flags);
    ssize_t sendto(int s, const void *dataptr, size_t len, int flags,
                   const struct sockaddr *to, socklen_t tolen);
    ssize_t write(int s, const void *dataptr, size_t len);
    ssize_t writev(int s, const struct iovec *iov, int iovcnt);
};

}
}
}

#endif /* _ICP_SOCKET_API_CLIENT_H_ */
