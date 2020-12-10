#include "dpdk.hpp"
#include <fcntl.h>
#include "socket/client/api_client.hpp"

namespace openperf::network::internal::drivers {

using client = socket::api::client;

static thread_local std::atomic_bool init_flag = false;

void dpdk::init()
{
    if (!init_flag.load()) client::instance().init(&init_flag);
};

bool dpdk::bind_to_device_required() { return true; };

bool dpdk::address_family_required() { return true; };

int dpdk::accept(int s, struct sockaddr* addr, socklen_t* addrlen, int flags)
{
    init();
    return client::instance().accept(s, addr, addrlen, flags);
};
int dpdk::bind(int s, const struct sockaddr* name, socklen_t namelen)
{
    init();
    return client::instance().bind(s, name, namelen);
};
int dpdk::shutdown(int s, int how)
{
    init();
    return client::instance().shutdown(s, how);
};
int dpdk::getpeername(int s, struct sockaddr* name, socklen_t* namelen)
{
    init();
    return client::instance().getpeername(s, name, namelen);
};
int dpdk::getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
    init();
    return client::instance().getsockname(s, name, namelen);
};
int dpdk::getsockopt(
    int s, int level, int optname, void* optval, socklen_t* optlen)
{
    init();
    return client::instance().getsockopt(s, level, optname, optval, optlen);
};
int dpdk::setsockopt(
    int s, int level, int optname, const void* optval, socklen_t optlen)
{
    init();
    return client::instance().setsockopt(s, level, optname, optval, optlen);
};
int dpdk::close(int s)
{
    init();
    return client::instance().close(s);
};
int dpdk::connect(int s, const struct sockaddr* name, socklen_t namelen)
{
    init();
    return client::instance().connect(s, name, namelen);
};
int dpdk::listen(int s, int backlog)
{
    init();
    return client::instance().listen(s, backlog);
};
int dpdk::socket(int domain, int type, int protocol)
{
    init();
    return client::instance().socket(domain, type, protocol);
};
int dpdk::fcntl(int fd, int cmd, ...)
{
    init();
    int result = 0;
    va_list ap;
    va_start(ap, cmd);

    switch (cmd) {
    case F_GETFD:
    case F_GETFL:
    case F_GETOWN:
        result = client::instance().fcntl(fd, cmd);
        break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETFL:
    case F_SETOWN:
        result = client::instance().fcntl(fd, cmd, va_arg(ap, int));
        break;
    default:
        result = client::instance().fcntl(fd, cmd, va_arg(ap, void*));
    }
    va_end(ap);
    return (result);
};

ssize_t dpdk::read(int s, void* mem, size_t len)
{
    init();
    return client::instance().read(s, mem, len);
};
ssize_t dpdk::readv(int s, const struct iovec* iov, int iovcnt)
{
    init();
    return client::instance().readv(s, iov, iovcnt);
}
ssize_t dpdk::recv(int s, void* mem, size_t len, int flags)
{
    init();
    return client::instance().recv(s, mem, len, flags);
};
ssize_t dpdk::recvfrom(int s,
                       void* mem,
                       size_t len,
                       int flags,
                       struct sockaddr* from,
                       socklen_t* fromlen)
{
    init();
    return client::instance().recvfrom(s, mem, len, flags, from, fromlen);
};
ssize_t dpdk::recvmsg(int s, struct msghdr* message, int flags)
{
    init();
    return client::instance().recvmsg(s, message, flags);
};

ssize_t dpdk::send(int s, const void* dataptr, size_t len, int flags)
{
    init();
    return client::instance().send(s, dataptr, len, flags);
};
ssize_t dpdk::sendmsg(int s, const struct msghdr* message, int flags)
{
    init();
    return client::instance().sendmsg(s, message, flags);
};
ssize_t dpdk::sendto(int s,
                     const void* dataptr,
                     size_t len,
                     int flags,
                     const struct sockaddr* to,
                     socklen_t tolen)
{
    init();
    return client::instance().sendto(s, dataptr, len, flags, to, tolen);
};
ssize_t dpdk::write(int s, const void* dataptr, size_t len)
{
    init();
    return client::instance().write(s, dataptr, len);
};
ssize_t dpdk::writev(int s, const struct iovec* iov, int iovcnt)
{
    init();
    return client::instance().writev(s, iov, iovcnt);
};

} // namespace openperf::network::internal::drivers
