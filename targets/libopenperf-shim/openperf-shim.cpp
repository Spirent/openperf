#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <arpa/inet.h>

#include "socket/client/api_client.hpp"
#include "libc_wrapper.hpp"

/* Don't depend on being in a Linux environment */
#ifndef AF_PACKET
#define AF_PACKET 17
#endif

struct linux_sockaddr_ll
{
    uint16_t sll_family;
    uint16_t sll_protocol;
    int sll_ifindex;
    uint16_t sll_hatype;
    uint8_t sll_pkttype;
    uint8_t sll_halen;
    uint8_t sll_addr[8];
};

static void openperf_shim_init() __attribute__((constructor));

/*
 * This flag is used to prevent libc calls from the client constructor
 * from instantiating a new client instance. Intercepted socket
 * calls MUST verify this variable is true before attempting to call
 * functions in the client instance.
 */
static std::atomic_bool client_initialized = ATOMIC_VAR_INIT(false);

static bool client_trace = false;

static constexpr std::string_view client_trace_var = "OP_TRACE\0";
static constexpr std::string_view client_bindtodev_var = "OP_BINDTODEVICE\0";

void openperf_shim_init()
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    libc.init();

    auto& client = openperf::socket::api::client::instance();
    client.init(&client_initialized);

    auto envp = std::getenv(client_trace_var.data());
    if (envp != nullptr && std::strncmp(envp, "0", 1) != 0) {
        client_trace = true;
    }
}

template <typename T> std::string to_hex_string(T x)
{
    std::ostringstream out;
    out << "0x" << std::hex << std::setw(2 * sizeof(T)) << std::setfill('0')
        << x;
    return (out.str());
}

/* Provide a pretty printing overload for sockaddr structures */
std::ostream& operator<<(std::ostream& out, const sockaddr* sa)
{
    if (!sa) {
        out << "NULL";
        return (out);
    }
    switch (sa->sa_family) {
    case AF_INET: {
        auto buffer = std::array<char, INET_ADDRSTRLEN>{"X.X.X.X"};
        auto* sin = reinterpret_cast<const sockaddr_in*>(sa);
        inet_ntop(AF_INET, &sin->sin_addr, buffer.data(), buffer.size());
        out << "AF_INET:" << buffer.data() << ":" << ntohs(sin->sin_port);
        break;
    }
    case AF_INET6: {
        auto buffer = std::array<char, INET6_ADDRSTRLEN>{"X::X"};
        auto* sin6 = reinterpret_cast<const sockaddr_in6*>(sa);
        inet_ntop(AF_INET6, &sin6->sin6_addr, buffer.data(), buffer.size());
        out << "AF_INET6:" << buffer.data() << ":" << ntohs(sin6->sin6_port);
        break;
    }
    case AF_PACKET: {
        auto* sll = reinterpret_cast<const linux_sockaddr_ll*>(sa);
        out << "AF_PACKET:" << to_hex_string(ntohs(sll->sll_protocol)) << ":"
            << sll->sll_ifindex;
        break;
    }
    default:
        out << "sockaddr (family = " << sa->sa_family << ")";
        break;
    }

    return (out);
}

template <typename Function, typename Object, typename Arg, typename... Args>
auto call_and_log_function(const char* function_name,
                           Function&& f,
                           Object&& o,
                           Arg&& arg,
                           Args&&... args)
{
    std::cerr << function_name << "(" << arg;
    ((std::cerr << ", " << args), ...);
    std::cerr << ") = " << std::flush;
    auto result = std::invoke(std::forward<Function>(f),
                              std::forward<Object>(o),
                              std::forward<Arg>(arg),
                              std::forward<Args>(args)...);
    std::cerr << result;

    if (result < 0) { std::cerr << " (errno = " << errno << ")"; }

    std::cerr << std::endl;

    return (result);
}

#define client_call(function, ...)                                             \
    (client_trace                                                              \
         ? call_and_log_function(#function,                                    \
                                 &openperf::socket::api::client::function,     \
                                 client,                                       \
                                 __VA_ARGS__)                                  \
         : client.function(__VA_ARGS__))

extern "C" {

int accept(int s, struct sockaddr* addr, socklen_t* addrlen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.accept(s, addr, addrlen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(accept, s, addr, addrlen, 0)
                                : libc.accept(s, addr, addrlen));
}

int accept4(int s, struct sockaddr* addr, socklen_t* addrlen, int flags)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.accept(s, addr, addrlen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(accept, s, addr, addrlen, flags)
                                : libc.accept(s, addr, addrlen));
}

int bind(int s, const struct sockaddr* name, socklen_t namelen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.bind(s, name, namelen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(bind, s, name, namelen)
                                : libc.bind(s, name, namelen));
}

int shutdown(int s, int how)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.shutdown(s, how)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(shutdown, s, how)
                                : libc.shutdown(s, how));
}

int getpeername(int s, struct sockaddr* name, socklen_t* namelen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.getpeername(s, name, namelen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(getpeername, s, name, namelen)
                                : libc.getpeername(s, name, namelen));
}

int getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.getsockname(s, name, namelen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(getsockname, s, name, namelen)
                                : libc.getsockname(s, name, namelen));
}

int getsockopt(int s, int level, int optname, void* optval, socklen_t* optlen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.getsockopt(s, level, optname, optval, optlen));
    }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s)
                ? client_call(getsockopt, s, level, optname, optval, optlen)
                : libc.getsockopt(s, level, optname, optval, optlen));
}

int setsockopt(
    int s, int level, int optname, const void* optval, socklen_t optlen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.setsockopt(s, level, optname, optval, optlen));
    }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s)
                ? client_call(setsockopt, s, level, optname, optval, optlen)
                : libc.setsockopt(s, level, optname, optval, optlen));
}

int close(int s)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.close(s)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(close, s) : libc.close(s));
}

int connect(int s, const struct sockaddr* name, socklen_t namelen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.connect(s, name, namelen)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(connect, s, name, namelen)
                                : libc.connect(s, name, namelen));
}

int listen(int s, int backlog)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.listen(s, backlog)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(listen, s, backlog)
                                : libc.listen(s, backlog));
}

int libc_socket(int domain, int type, int protocol)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    return (libc.socket(domain, type, protocol));
}

int socket(int domain, int type, int protocol)
{
    if (domain != AF_INET && domain != AF_INET6 && domain != AF_PACKET) {
        return (libc_socket(domain, type, protocol));
    }

    auto& client = openperf::socket::api::client::instance();

    int s = client_call(socket, domain, type, protocol);
    if (s >= 0) {
        if (auto envp = std::getenv(client_bindtodev_var.data());
            envp != nullptr) {
            /*
             * Close socket and return -1 on BINDTODEVICE failure.
             */
            if (client_call(setsockopt,
                            s,
                            SOL_SOCKET,
                            SO_BINDTODEVICE,
                            envp,
                            strlen(envp))
                < 0) {
                client_call(close, s);
                return (-1);
            }
        }
    }
    return (s);
}

int fcntl(int s, int cmd, ...)
{
    /*
     * XXX: this cmd list is incomplete as Linux has a lot of wonky
     * options.
     * Also...  if you can think of a concise, clean way to handle
     * all of these options and the variable argument list without
     * duplication while avoiding the use of _goto_, please do so!
     */

    int result = 0;
    va_list ap;
    va_start(ap, cmd);

    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
    libc_fcntl:
        switch (cmd) {
        case F_GETFD:
        case F_GETFL:
        case F_GETOWN:
            result = libc.fcntl(s, cmd);
            break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETFL:
        case F_SETOWN:
            result = libc.fcntl(s, cmd, va_arg(ap, int));
            break;
        default:
            result = libc.fcntl(s, cmd, va_arg(ap, void*));
        }
        va_end(ap);
        return (result);
    }

    auto& client = openperf::socket::api::client::instance();
    if (!client.is_socket(s) || (cmd != F_GETFL && cmd != F_SETFL)) {
        goto libc_fcntl;
    }

    if (cmd == F_GETFL) {
        result = client_call(fcntl, s, cmd);
    } else {
        auto flags = va_arg(ap, int);
        result =
            (libc.fcntl(s, cmd, flags) || client_call(fcntl, s, cmd, flags));
    }

    va_end(ap);
    return (result);
}

int ioctl(int s, unsigned long req, ...)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
        va_list ap;
        va_start(ap, req);
        auto result = libc.ioctl(s, req, va_arg(ap, void*));
        va_end(ap);
        return (result);
    }

    auto& client = openperf::socket::api::client::instance();
    va_list ap;
    va_start(ap, req);
    auto result = client.is_socket(s)
                      ? client_call(ioctl, s, req, va_arg(ap, void*))
                      : libc.ioctl(s, req, va_arg(ap, void*));
    va_end(ap);
    return (result);
}

/* Receive functions */
ssize_t read(int s, void* mem, size_t len)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.read(s, mem, len)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(read, s, mem, len)
                                : libc.read(s, mem, len));
}

ssize_t readv(int s, const struct iovec* iov, int iovcnt)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.readv(s, iov, iovcnt)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(readv, s, iov, iovcnt)
                                : libc.readv(s, iov, iovcnt));
}

ssize_t recv(int s, void* mem, size_t len, int flags)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.recv(s, mem, len, flags)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(recv, s, mem, len, flags)
                                : libc.recv(s, mem, len, flags));
}

ssize_t recvfrom(int s,
                 void* mem,
                 size_t len,
                 int flags,
                 struct sockaddr* from,
                 socklen_t* fromlen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.recvfrom(s, mem, len, flags, from, fromlen));
    }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s)
                ? client_call(recvfrom, s, mem, len, flags, from, fromlen)
                : libc.recvfrom(s, mem, len, flags, from, fromlen));
}

ssize_t recvmsg(int s, struct msghdr* message, int flags)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.recvmsg(s, message, flags)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(recvmsg, s, message, flags)
                                : libc.recvmsg(s, message, flags));
}

/* Transmit functions */
ssize_t send(int s, const void* dataptr, size_t len, int flags)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.send(s, dataptr, len, flags)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(send, s, dataptr, len, flags)
                                : libc.send(s, dataptr, len, flags));
}

ssize_t sendmsg(int s, const struct msghdr* message, int flags)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.sendmsg(s, message, flags)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(sendmsg, s, message, flags)
                                : libc.sendmsg(s, message, flags));
}

ssize_t sendto(int s,
               const void* dataptr,
               size_t len,
               int flags,
               const struct sockaddr* to,
               socklen_t tolen)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) {
        return (libc.sendto(s, dataptr, len, flags, to, tolen));
    }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s)
                ? client_call(sendto, s, dataptr, len, flags, to, tolen)
                : libc.sendto(s, dataptr, len, flags, to, tolen));
}

ssize_t write(int s, const void* dataptr, size_t len)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.write(s, dataptr, len)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(write, s, dataptr, len)
                                : libc.write(s, dataptr, len));
}

ssize_t writev(int s, const struct iovec* iov, int iovcnt)
{
    auto& libc = openperf::socket::libc::wrapper::instance();
    if (!client_initialized) { return (libc.writev(s, iov, iovcnt)); }

    auto& client = openperf::socket::api::client::instance();
    return (client.is_socket(s) ? client_call(writev, s, iov, iovcnt)
                                : libc.writev(s, iov, iovcnt));
}
}
