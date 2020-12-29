#include "kernel.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <cstdarg>
#include <sys/uio.h>
#include <netinet/in.h>
#include <net/if.h>
#include <utility>

namespace openperf::network::internal::drivers {

int kernel::accept(int s, struct sockaddr* addr, socklen_t* addrlen, int flags)
{
    return ::accept4(s, addr, addrlen, flags);
};
int kernel::bind(int s, const struct sockaddr* name, socklen_t namelen)
{
    return ::bind(s, name, namelen);
};
int kernel::shutdown(int s, int how) { return ::shutdown(s, how); };
int kernel::getpeername(int s, struct sockaddr* name, socklen_t* namelen)
{
    return ::getpeername(s, name, namelen);
};
int kernel::getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
    return ::getsockname(s, name, namelen);
};
int kernel::getsockopt(
    int s, int level, int optname, void* optval, socklen_t* optlen)
{
    return ::getsockopt(s, level, optname, optval, optlen);
};
int kernel::setsockopt(
    int s, int level, int optname, const void* optval, socklen_t optlen)
{
    return ::setsockopt(s, level, optname, optval, optlen);
};
int kernel::close(int s) { return ::close(s); };
int kernel::connect(int s, const struct sockaddr* name, socklen_t namelen)
{
    return ::connect(s, name, namelen);
};
int kernel::listen(int s, int backlog) { return ::listen(s, backlog); };
int kernel::socket(int domain, int type, int protocol)
{
    return ::socket(domain, type, protocol);
};
int kernel::fcntl(int fd, int cmd, ...)
{
    int result = 0;
    va_list ap;
    va_start(ap, cmd);

    switch (cmd) {
    case F_GETFD:
    case F_GETFL:
    case F_GETOWN:
        result = ::fcntl(fd, cmd);
        break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
        result = ::fcntl(fd, cmd, va_arg(ap, int));
        break;
    default:
        result = ::fcntl(fd, cmd, va_arg(ap, void*));
    }
    va_end(ap);
    return (result);
};

ssize_t kernel::read(int s, void* mem, size_t len)
{
    return ::read(s, mem, len);
};
ssize_t kernel::readv(int s, const struct iovec* iov, int iovcnt)
{
    return ::readv(s, iov, iovcnt);
}
ssize_t kernel::recv(int s, void* mem, size_t len, int flags)
{
    return ::recv(s, mem, len, flags);
};
ssize_t kernel::recvfrom(int s,
                         void* mem,
                         size_t len,
                         int flags,
                         struct sockaddr* from,
                         socklen_t* fromlen)
{
    return ::recvfrom(s, mem, len, flags, from, fromlen);
};
ssize_t kernel::recvmsg(int s, struct msghdr* message, int flags)
{
    return ::recvmsg(s, message, flags);
};

ssize_t kernel::send(int s, const void* dataptr, size_t len, int flags)
{
    return ::send(s, dataptr, len, flags);
};
ssize_t kernel::sendmsg(int s, const struct msghdr* message, int flags)
{
    return ::sendmsg(s, message, flags);
};
ssize_t kernel::sendto(int s,
                       const void* dataptr,
                       size_t len,
                       int flags,
                       const struct sockaddr* to,
                       socklen_t tolen)
{
    return ::sendto(s, dataptr, len, flags, to, tolen);
};
ssize_t kernel::write(int s, const void* dataptr, size_t len)
{
    return ::write(s, dataptr, len);
};
ssize_t kernel::writev(int s, const struct iovec* iov, int iovcnt)
{
    return ::writev(s, iov, iovcnt);
};

unsigned int kernel::if_nametoindex(const char* ifname)
{
    return ::if_nametoindex(ifname);
}

} // namespace openperf::network::internal::drivers
