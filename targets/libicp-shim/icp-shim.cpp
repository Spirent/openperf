#include <atomic>

#include "socket/client/api_client.h"
#include "libc_wrapper.h"

static void icp_shim_init() __attribute__((constructor));

static std::atomic_bool client_initialized = ATOMIC_VAR_INIT(false);

//#define SHIM_TRACE(format, ...) fprintf(stderr, format, ##__VA_ARGS__);

#define SHIM_TRACE(fomat, ...)

void icp_shim_init()
{
    auto& libc = icp::socket::libc::wrapper::instance();
    libc.init();

    auto& client = icp::socket::api::client::instance();
    client.init();
    client_initialized = true;
}
extern "C" {

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    SHIM_TRACE("accept(%d, %p, %u)n", s, (void*)addr, addrlen ? *addrlen : 0);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.accept(s, addr, addrlen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.accept(s, addr, addrlen)
            : libc.accept(s, addr, addrlen));
}

int bind(int s, const struct sockaddr *name, socklen_t namelen)
{
    SHIM_TRACE("bind(%d, %p, %d)\n", s, (void*)name, namelen);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.bind(s, name, namelen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.bind(s, name, namelen)
            : libc.bind(s, name, namelen));
}

int shutdown(int s, int how)
{
    SHIM_TRACE("shutdown(%d, %d)\n", s, how);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.shutdown(s, how));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.shutdown(s, how)
            : libc.shutdown(s, how));
}

int getpeername(int s, struct sockaddr *name, socklen_t *namelen)
{
    SHIM_TRACE("getpeername(%d, %p, %u)\n", s, (void*)name, namelen ? *namelen : 0);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.getpeername(s, name, namelen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.getpeername(s, name, namelen)
            : libc.getpeername(s, name, namelen));
}

int getsockname(int s, struct sockaddr *name, socklen_t *namelen)
{
    SHIM_TRACE("getsockname(%d, %p, %u)\n", s, (void*)name, *namelen);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.getsockname(s, name, namelen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.getsockname(s, name, namelen)
            : libc.getsockname(s, name, namelen));
}

int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen)
{
    SHIM_TRACE("getsockopt(%d, %d, %d, %p, %u)\n", s, level, optname, optval,
               optlen ? *optlen : 0);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.getsockopt(s, level, optname, optval, optlen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.getsockopt(s, level, optname, optval, optlen)
            : libc.getsockopt(s, level, optname, optval, optlen));
}

int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen)
{
    SHIM_TRACE("setsockopt(%d, %d, %d, %p, %u)\n", s, level, optname, optval, optlen);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.setsockopt(s, level, optname, optval, optlen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.setsockopt(s, level, optname, optval, optlen)
            : libc.setsockopt(s, level, optname, optval, optlen));
}

int close(int s)
{
    SHIM_TRACE("close(%d)\n", s);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.close(s));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.close(s)
            : libc.close(s));
}

int connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    SHIM_TRACE("connect(%d, %p, %d)\n", s, (void*)name, namelen);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.connect(s, name, namelen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.connect(s, name, namelen)
            : libc.connect(s, name, namelen));
}

int listen(int s, int backlog)
{
    SHIM_TRACE("listen(%d, %d)\n", s, backlog);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.listen(s, backlog));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.listen(s, backlog)
            : libc.listen(s, backlog));
}

int libc_socket(int domain, int type, int protocol)
{
    SHIM_TRACE("libc_socket(%d, %d, %d)\n", domain, type, protocol);

    auto& libc = icp::socket::libc::wrapper::instance();
    return (libc.socket(domain, type, protocol));
}

int socket(int domain, int type, int protocol)
{
    if (domain != AF_INET && domain != AF_INET6) {
        return (libc_socket(domain, type, protocol));
    }

    SHIM_TRACE("socket(%d, %d, %d)\n", domain, type, protocol);

    auto& client = icp::socket::api::client::instance();
    return (client.socket(domain, type, protocol));
}

int ioctl(int s, long cmd, void *argp)
{
    SHIM_TRACE("ioctl(%d, %ld, %p)\n", s, cmd, argp);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.ioctl(s, cmd, argp));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.ioctl(s, cmd, argp)
            : libc.ioctl(s, cmd, argp));
}

/* Receive functions */
ssize_t read(int s, void *mem, size_t len)
{
    SHIM_TRACE("read(%d, %p, %zu)\n", s, mem, len);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.read(s, mem, len));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.read(s, mem, len)
            : libc.read(s, mem, len));
}

ssize_t readv(int s, const struct iovec *iov, int iovcnt)
{
    SHIM_TRACE("readv(%d, %p, %d)\n", s, (void*)iov, iovcnt);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.readv(s, iov, iovcnt));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.readv(s, iov, iovcnt)
            : libc.readv(s, iov, iovcnt));
}

ssize_t recv(int s, void *mem, size_t len, int flags)
{
    SHIM_TRACE("recv(%d, %p, %zu, %d)\n", s, mem, len, flags);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.recv(s, mem, len, flags));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.recv(s, mem, len, flags)
            : libc.recv(s, mem, len, flags));
}

ssize_t recvfrom(int s, void *mem, size_t len, int flags,
                 struct sockaddr *from, socklen_t *fromlen)
{
    SHIM_TRACE("recvfrom(%d, %p, %zu, %d, %p, %u)\n", s, mem, len, flags, (void*)from,
               fromlen ? *fromlen : 0);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.recvfrom(s, mem, len, flags, from, fromlen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.recvfrom(s, mem, len, flags, from, fromlen)
            : libc.recvfrom(s, mem, len, flags, from, fromlen));
}

ssize_t recvmsg(int s, struct msghdr *message, int flags)
{
    SHIM_TRACE("recvmsg(%d, %p, %d)\n", s, (void*)message, flags);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.recvmsg(s, message, flags));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.recvmsg(s, message, flags)
            : libc.recvmsg(s, message, flags));
}

/* Transmit functions */
ssize_t send(int s, const void *dataptr, size_t len, int flags)
{
    SHIM_TRACE("send(%d, %p, %zu, %d)\n", s, dataptr, len, flags);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.send(s, dataptr, len, flags));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.send(s, dataptr, len, flags)
            : libc.send(s, dataptr, len, flags));
}

ssize_t sendmsg(int s, const struct msghdr *message, int flags)
{
    SHIM_TRACE("sendsg(%d, %p, %d)\n", s, (void*)message, flags);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.sendmsg(s, message, flags));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.sendmsg(s, message, flags)
            : libc.sendmsg(s, message, flags));
}

ssize_t sendto(int s, const void *dataptr, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen)
{
    SHIM_TRACE("sendto(%d, %p, %zu, %d, %p, %u)\n", s, dataptr, len, flags, (void*)to, tolen);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.sendto(s, dataptr, len, flags, to, tolen));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.sendto(s, dataptr, len, flags, to, tolen)
            : libc.sendto(s, dataptr, len, flags, to, tolen));
}

ssize_t write(int s, const void *dataptr, size_t len)
{
    SHIM_TRACE("write(%d, %p, %zu)\n", s, (void*)dataptr, len);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.write(s, dataptr, len));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.write(s, dataptr, len)
            : libc.write(s, dataptr, len));
}

ssize_t writev(int s, const struct iovec *iov, int iovcnt)
{
    SHIM_TRACE("writev(%d, %p, %d)\n", s, (void*)iov, iovcnt);

    auto& libc = icp::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.writev(s, iov, iovcnt));
    }

    auto& client = icp::socket::api::client::instance();
    return (client.is_socket(s)
            ? client.writev(s, iov, iovcnt)
            : libc.writev(s, iov, iovcnt));
}

}
