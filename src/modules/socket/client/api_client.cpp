#include <cassert>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <numeric>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket/api.hpp"
#include "socket/process_control.hpp"
#include "socket/client/api_client.hpp"
#include "socket/client/io_channel_wrapper.hpp"

namespace openperf::socket::api {

using io_channel_wrapper = openperf::socket::client::io_channel_wrapper;
struct ided_channel
{
    socket_id id;
    io_channel_wrapper channel;
};
static std::unordered_map<int, ided_channel> socket_channels;

client::client()
    : m_uuid(core::uuid::random())
    , m_sock(api::client_socket(to_string(m_uuid)), api::socket_type)
{
    auto server = sockaddr_un{.sun_family = AF_UNIX};
    if (auto envp = std::getenv("OP_PREFIX"); envp != nullptr) {
        std::strncpy(server.sun_path,
                     (api::server_socket() + "." + std::string(envp)).c_str(),
                     sizeof(server.sun_path));
    } else {
        std::strncpy(server.sun_path,
                     api::server_socket().c_str(),
                     sizeof(server.sun_path));
    }

    if (::connect(
            m_sock.get(), reinterpret_cast<sockaddr*>(&server), sizeof(server))
        == -1) {
        throw std::runtime_error("Could not connect to socket server: "
                                 + std::string(strerror(errno)));
    }
}

client::~client() { *m_init_flag = false; }

static api::reply_msg submit_request(int sockfd,
                                     const api::request_msg& request)
{
    if (send(sockfd,
             const_cast<void*>(reinterpret_cast<const void*>(&request)),
             sizeof(request),
             0)
        == -1) {
        return (tl::make_unexpected(errno));
    }

    /* Setup to receive a message */
    api::reply_msg reply;

    struct iovec iov = {.iov_base = reinterpret_cast<void*>(&reply),
                        .iov_len = sizeof(reply)};

    /* Create a properly aligned data buffer for our cmsg data */
    union
    {
        char data[CMSG_SPACE(sizeof(api::socket_fd_pair))];
        struct cmsghdr align;
    } control;

    struct msghdr msg = {.msg_iov = &iov,
                         .msg_iovlen = 1,
                         .msg_control = control.data,
                         .msg_controllen = sizeof(control.data)};

    if (recvmsg(sockfd, &msg, 0) == -1) { return (tl::make_unexpected(errno)); }

    if (struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr) {
        if (cmsg->cmsg_len == CMSG_LEN(sizeof(api::socket_fd_pair))
            && cmsg->cmsg_level == SOL_SOCKET
            && cmsg->cmsg_type == SCM_RIGHTS) {
            /*
             * Message contains a fd, update the message sockfd value so the
             * client can use it directly.
             */
            set_message_fds(
                reply,
                *(reinterpret_cast<api::socket_fd_pair*>(CMSG_DATA(cmsg))));
        }
    }

    return (reply);
}

/* Send a hello message to server; wait for reply */
void client::init(std::atomic_bool* init_flag)
{
    api::request_msg request =
        api::request_init{.pid = getpid(), .tid = m_uuid};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        fprintf(stderr, "RPC error: %s\n", strerror(reply.error()));
        return;
    }

    auto& init = std::get<api::reply_init>(*reply);

    if (init.shm_info) {
        /* map shared address memory */
        auto shm_info = *init.shm_info;
        m_shm = std::make_unique<memory::shared_segment>(shm_info.name,
                                                         shm_info.size);

        /* enable ptrace from server side */
        process_control::enable_ptrace(stderr, init.pid);
    }

    m_init_flag = init_flag;
    *m_init_flag = true;
}

bool client::is_socket(int s) { return (socket_channels.count(s)); }

int client::accept(int s, struct sockaddr* addr, socklen_t* addrlen, int flags)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    auto wait = channel.wait_readable();
    if (!wait) {
        errno = wait.error();
        return (-1);
    }

    api::request_msg request =
        api::request_accept{.id = id,
                            .flags = flags,
                            .addr = addr,
                            .addrlen = (addrlen ? *addrlen : 0)};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    auto& accept = std::get<api::reply_accept>(*reply);

    /* Record socket data for future use */
    auto newresult = socket_channels.emplace(
        accept.fd_pair.client_fd,
        ided_channel{
            accept.id,
            io_channel_wrapper(to_pointer(accept.channel, m_shm->base()),
                               accept.fd_pair.client_fd,
                               accept.fd_pair.server_fd)});

    if (!newresult.second) {
        errno = ENOBUFS;
        return (-1);
    }

    if (addrlen) { *addrlen = accept.addrlen; }

    /* Give the client their fd */
    return (accept.fd_pair.client_fd);
}

int client::bind(int s, const struct sockaddr* name, socklen_t namelen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_bind{.id = id, .name = name, .namelen = namelen};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    assert(std::holds_alternative<api::reply_success>(*reply));
    return (0);
}

int client::shutdown(int s, int how)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request = api::request_shutdown{.id = id, .how = how};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    assert(std::holds_alternative<api::reply_success>(*reply));
    return (0);
}

int client::getpeername(int s, struct sockaddr* name, socklen_t* namelen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_getpeername{.id = id, .name = name, .namelen = *namelen};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    *namelen = std::get<api::reply_socklen>(*reply).length;
    return (0);
}

int client::getsockname(int s, struct sockaddr* name, socklen_t* namelen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_getsockname{.id = id, .name = name, .namelen = *namelen};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    *namelen = std::get<api::reply_socklen>(*reply).length;
    return (0);
}

int client::getsockopt(
    int s, int level, int optname, void* optval, socklen_t* optlen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_getsockopt{.id = id,
                                .level = level,
                                .optname = optname,
                                .optval = optval,
                                .optlen = (optlen ? *optlen : 0)};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    if (optlen) { *optlen = std::get<api::reply_socklen>(*reply).length; }
    return (0);
}

int client::setsockopt(
    int s, int level, int optname, const void* optval, socklen_t optlen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request = api::request_setsockopt{.id = id,
                                                       .level = level,
                                                       .optname = optname,
                                                       .optval = optval,
                                                       .optlen = optlen};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    assert(std::holds_alternative<api::reply_success>(*reply));
    return (0);
}

int client::close(int s)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        /* XXX: should hand off to libc close */
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request = api::request_close{.id = id};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    socket_channels.erase(result);
    return (0);
}

int client::connect(int s, const struct sockaddr* name, socklen_t namelen)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_connect{.id = id, .name = name, .namelen = namelen};

    /* Connecting sockets should be non-writable, so block the socket */
    auto block = channel.block_writes();
    if (!block) {
        errno = block.error();
        return (-1);
    }

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    /* We have two possible non-error replies: success or working */
    if (std::holds_alternative<api::reply_success>(*reply)) { return (0); }

    assert(std::holds_alternative<api::reply_working>(*reply));

    /*
     * At this point the connect is in progress in the stack.
     * If we have a non-blocking socket, let the caller know we're working on
     * it.
     */
    if (channel.flags() & SOCK_NONBLOCK) {
        errno = EINPROGRESS;
        return (-1);
    }

    /* Blocking socket; wait for the socket to become writable */
    auto wait = channel.wait_writable();
    if (!wait) {
        errno = wait.error();
        return (-1);
    }

    return (channel.error());
}

int client::listen(int s, int backlog)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    api::request_msg request =
        api::request_listen{.id = id, .backlog = backlog};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    assert(std::holds_alternative<api::reply_success>(*reply));
    return (0);
}

int client::socket(int domain, int type, int protocol)
{
    api::request_msg request = api::request_socket{
        .domain = domain, .type = type, .protocol = protocol};

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    auto& socket = std::get<api::reply_socket>(*reply);

    /* Record socket data for future use */
    auto result = socket_channels.emplace(
        socket.fd_pair.client_fd,
        ided_channel{
            socket.id,
            io_channel_wrapper(to_pointer(socket.channel, m_shm->base()),
                               socket.fd_pair.client_fd,
                               socket.fd_pair.server_fd)});

    if (!result.second) {
        errno = ENOBUFS;
        return (-1);
    }

    /* Give the client their fd */
    return (socket.fd_pair.client_fd);
}

int client::fcntl(int s, int cmd, ...)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;

    va_list ap;
    va_start(ap, cmd);

    int to_return = 0;

    switch (cmd) {
    case F_GETFL:
        to_return = channel.flags();
        break;
    case F_SETFL:
        to_return = channel.flags(va_arg(ap, int));
        break;
    case F_GETFD:
    case F_GETOWN:
        to_return = ::fcntl(s, cmd);
        break;
    case F_DUPFD:
    case F_DUPFD_CLOEXEC:
    case F_SETFD:
    case F_SETOWN:
        to_return = ::fcntl(s, cmd, va_arg(ap, int));
        break;
    default:
        to_return = ::fcntl(s, cmd, va_arg(ap, void*));
    }

    va_end(ap);
    return (to_return);
}

int client::ioctl(int s, unsigned long req, ...)
{
    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;

    va_list ap;
    va_start(ap, req);

    api::request_msg request =
        api::request_ioctl{.id = id, .request = req, .argp = va_arg(ap, void*)};

    auto reply = submit_request(m_sock.get(), request);
    va_end(ap);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    assert(std::holds_alternative<api::reply_success>(*reply));
    return (0);
}

/***
 * Receive functions
 ***/
ssize_t client::read(int s, void* mem, size_t len)
{
    return (recv(s, mem, len, 0));
}

ssize_t client::readv(int s, const struct iovec* iov, int iovcnt)
{
    auto msg =
        msghdr{.msg_iov = const_cast<iovec*>(iov),
               .msg_iovlen = static_cast<decltype(msghdr::msg_iovlen)>(iovcnt)};

    return (recvmsg(s, &msg, 0));
}

ssize_t client::recv(int s, void* mem, size_t len, int flags)
{
    return (recvfrom(s, mem, len, flags, nullptr, nullptr));
}

ssize_t client::recvfrom(int s,
                         void* mem,
                         size_t len,
                         int flags,
                         struct sockaddr* from,
                         socklen_t* fromlen)
{
    auto iov = iovec{.iov_base = mem, .iov_len = len};

    auto msg = msghdr{.msg_name = from,
                      .msg_namelen = (fromlen ? *fromlen : 0),
                      .msg_iov = &iov,
                      .msg_iovlen = 1};

    auto to_return = recvmsg(s, &msg, flags);
    if (fromlen && to_return != -1) *fromlen = msg.msg_namelen;
    return (to_return);
}

ssize_t client::recvmsg(int s, struct msghdr* message, int flags)
{
    (void)flags; /* TODO */

    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    auto recv_result =
        channel.recv(message->msg_iov,
                     message->msg_iovlen,
                     flags,
                     reinterpret_cast<sockaddr*>(message->msg_name),
                     &message->msg_namelen);
    if (!recv_result) {
        errno = recv_result.error();
        return (-1);
    }

    return (*recv_result);
}

/***
 * Transmit functions
 ***/
ssize_t client::send(int s, const void* dataptr, size_t len, int flags)
{
    return (sendto(s, dataptr, len, flags, nullptr, 0));
}

ssize_t client::sendmsg(int s, const struct msghdr* message, int flags)
{
    (void)flags; /* TODO */

    auto result = socket_channels.find(s);
    if (result == socket_channels.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& [id, channel] = result->second;
    auto send_result =
        channel.send(message->msg_iov,
                     message->msg_iovlen,
                     flags,
                     reinterpret_cast<const sockaddr*>(message->msg_name));
    if (!send_result) {
        errno = send_result.error();
        return (-1);
    }

    return (*send_result);
}

ssize_t client::sendto(int s,
                       const void* dataptr,
                       size_t len,
                       int flags,
                       const struct sockaddr* to,
                       socklen_t tolen)
{
    auto iov = iovec{.iov_base = const_cast<void*>(dataptr), .iov_len = len};

    auto msg = msghdr{.msg_name = const_cast<sockaddr*>(to),
                      .msg_namelen = tolen,
                      .msg_iov = &iov,
                      .msg_iovlen = 1};

    return (sendmsg(s, &msg, flags));
}

ssize_t client::write(int s, const void* dataptr, size_t len)
{
    return (send(s, dataptr, len, 0));
}

ssize_t client::writev(int s, const struct iovec* iov, int iovcnt)
{
    auto msg =
        msghdr{.msg_iov = const_cast<iovec*>(iov),
               .msg_iovlen = static_cast<decltype(msghdr::msg_iovlen)>(iovcnt)};

    return (sendmsg(s, &msg, 0));
}

} // namespace openperf::socket::api
