#ifndef _OP_SHIM_LIBC_WRAPPER_HPP_
#define _OP_SHIM_LIBC_WRAPPER_HPP_

#include <sys/socket.h>

namespace openperf {
namespace socket {
namespace libc {

template <typename T> class singleton
{
public:
    static T& instance()
    {
        static T instance;
        return instance;
    }

    singleton(const singleton&) = delete;
    singleton& operator=(const singleton) = delete;

protected:
    singleton(){};
};

struct wrapper : singleton<wrapper>
{
    int (*accept)(int s, struct sockaddr* addr, socklen_t* addren);
    int (*bind)(int s, const struct sockaddr* name, socklen_t namelen);
    int (*shutdown)(int s, int how);
    int (*getpeername)(int s, struct sockaddr* name, socklen_t* namelen);
    int (*getsockname)(int s, struct sockaddr* name, socklen_t* namelen);
    int (*getsockopt)(int s, int level, int optname, void* optval,
                      socklen_t* optlen);
    int (*setsockopt)(int s, int level, int optname, const void* optval,
                      socklen_t optlen);
    int (*close)(int s);
    int (*connect)(int s, const struct sockaddr* name, socklen_t namelen);
    int (*listen)(int s, int backlog);
    int (*socket)(int domain, int type, int protocol);
    int (*fcntl)(int s, int cmd, ...);
    ssize_t (*read)(int s, void* mem, size_t len);
    ssize_t (*readv)(int s, const struct iovec* iov, int iovcnt);
    ssize_t (*recv)(int s, void* mem, size_t len, int flags);
    ssize_t (*recvfrom)(int s, void* mem, size_t len, int flags,
                        struct sockaddr* from, socklen_t* fromlen);
    ssize_t (*recvmsg)(int s, struct msghdr* message, int flags);
    ssize_t (*send)(int s, const void* dataptr, size_t len, int flags);
    ssize_t (*sendmsg)(int s, const struct msghdr* message, int flags);
    ssize_t (*sendto)(int s, const void* dataptr, size_t len, int flags,
                      const struct sockaddr* to, socklen_t tolen);
    ssize_t (*write)(int s, const void* dataptr, size_t len);
    ssize_t (*writev)(int s, const struct iovec* iov, int iovcnt);

    void init();
};

} // namespace libc
} // namespace socket
} // namespace openperf

#endif /* _OP_SHIM_LIBC_WRAPPER_HPP_ */
