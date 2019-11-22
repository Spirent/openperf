#include <cerrno>
#include <stdexcept>
#include <sys/un.h>

#include "socket/server/allocator.hpp"
#include "socket/server/api_handler.hpp"
#include "socket/server/socket_utils.hpp"
#include "core/op_core.h"
#include "utils/overloaded_visitor.hpp"

namespace openperf::socket::server {

api_handler::api_handler(event_loop& loop, const void* shm_base,
                         allocator& allocator, pid_t pid)
    : m_loop(loop)
    , m_shm_base(shm_base)
    , m_allocator(allocator)
    , m_pid(pid)
    , m_next_socket_id(0)
{}

api_handler::~api_handler()
{
    /* Remove any remaining fds in our loop that the client didn't explicitly
     * close. */
    for (auto fd : m_server_fds) { m_loop.del_callback(fd); }
}

static ssize_t send_reply(int sockfd, const sockaddr_un& client,
                          const api::reply_msg& reply)
{
    struct iovec iov = {
        .iov_base = const_cast<void*>(reinterpret_cast<const void*>(&reply)),
        .iov_len = sizeof(reply)};

    struct msghdr msg = {
        .msg_name = const_cast<void*>(reinterpret_cast<const void*>(&client)),
        .msg_namelen = sizeof(client),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    if (auto fd_pair = api::get_message_fds(reply)) {
        /* Create a properly aligned data buffer for our cmsg data */
        union
        {
            char data[CMSG_SPACE(sizeof(*fd_pair))];
            struct cmsghdr align;
        } control;

        msg.msg_control = control.data;
        msg.msg_controllen = sizeof(control.data);

        struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(*fd_pair));
        memcpy(CMSG_DATA(cmsg), &(*fd_pair), sizeof(*fd_pair));

        OP_LOG(OP_LOG_DEBUG, "Sending client_fd = %d, server_fd = %d\n",
               fd_pair->client_fd, fd_pair->server_fd);
    }

    return (sendmsg(sockfd, &msg, 0));
}

/**
 * Read data from clients to transmit to the stack.
 */
static int handle_socket_read(api_handler::event_loop&, std::any arg)
{
    auto socket = std::any_cast<generic_socket*>(arg);
    OP_LOG(OP_LOG_TRACE, "Transmit request for socket %p\n", (void*)socket);
    socket->handle_io();
    return (0);
}

static void handle_socket_delete(std::any arg)
{
    auto socket = std::any_cast<generic_socket*>(arg);
    delete socket;
}

api::reply_msg
api_handler::handle_request_accept(const api::request_accept& request)
{
    auto lookup = m_sockets.find(request.id);
    if (lookup == m_sockets.end()) { return (tl::make_unexpected(ENOTSOCK)); }

    auto& listen_socket = lookup->second;
    auto new_sock = listen_socket.handle_accept(request.flags);
    if (!new_sock) { return (tl::make_unexpected(new_sock.error())); }

    auto id = api::socket_id{{m_pid, m_next_socket_id++}};
    auto stored = m_sockets.emplace(id, std::move(*new_sock));

    if (!stored.second) { return (tl::make_unexpected(ENOMEM)); }

    auto& accept_socket = stored.first->second;
    auto channel = accept_socket.channel();

    m_server_fds.emplace(server_fd(channel));
    m_loop.add_callback(
        "socket io for fd = " + std::to_string(server_fd(channel)),
        server_fd(channel), handle_socket_read, handle_socket_delete,
        new generic_socket(accept_socket));

    /* We need the peer name as well */
    auto addr_request = api::request_getpeername{
        .id = id, .name = request.addr, .namelen = request.addrlen};

    auto reply = accept_socket.handle_request(addr_request);
    assert(reply); /* this should never fail */

    return (api::reply_accept{
        .id = id,
        .channel = api_channel_offset(channel, m_shm_base),
        .fd_pair =
            {
                .client_fd = client_fd(channel),
                .server_fd = server_fd(channel),
            },
        .addrlen = std::get<api::reply_socklen>(*reply).length});
}

api::reply_msg
api_handler::handle_request_close(const api::request_close& request)
{
    /* lookup socket to close */
    auto result = m_sockets.find(request.id);
    if (result == m_sockets.end()) { return (tl::make_unexpected(EINVAL)); }

    auto& socket = result->second;
    socket.handle_io(); /* flush the tx queue */

    auto channel = socket.channel();
    m_loop.del_callback(server_fd(channel));
    m_server_fds.erase(server_fd(channel));
    m_sockets.erase(result);
    return (api::reply_success());
}

api::reply_msg
api_handler::handle_request_socket(const api::request_socket& request)
{
    auto id = api::socket_id{{m_pid, m_next_socket_id++}};
    auto socket = make_socket(m_allocator, request.domain, request.type,
                              request.protocol);
    if (!socket) { return (tl::make_unexpected(socket.error())); }

    auto result = m_sockets.emplace(id, *socket);
    if (!result.second) { return (tl::make_unexpected(ENOMEM)); }

    /* Add I/O callback for socket writes from client to stack */
    auto channel = socket->channel();
    m_server_fds.emplace(server_fd(channel));
    m_loop.add_callback("socket io for fd = "
                            + std::to_string(server_fd(channel)),
                        server_fd(channel), handle_socket_read,
                        handle_socket_delete, new generic_socket(*socket));

    return (
        api::reply_socket{.id = id,
                          .channel = api_channel_offset(channel, m_shm_base),
                          .fd_pair = {
                              .client_fd = client_fd(channel),
                              .server_fd = server_fd(channel),
                          }});
}

int api_handler::handle_requests(int fd)
{
    api::request_msg request;
    ssize_t recv_or_err = 0;
    sockaddr_un client;
    socklen_t client_length = sizeof(client);
    while ((recv_or_err =
                recvfrom(fd, reinterpret_cast<void*>(&request), sizeof(request),
                         MSG_DONTWAIT, reinterpret_cast<sockaddr*>(&client),
                         &client_length))
           == sizeof(api::request_msg)) {

        auto reply = std::visit(
            utils::overloaded_visitor(
                [](const api::request_init&) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "init request received\n");
                    return (tl::make_unexpected(EINVAL));
                },
                [&](const api::request_accept& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "accept request received\n");
                    return (handle_request_accept(request));
                },
                [&](const api::request_bind& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "bind request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_shutdown& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "shutdown request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_getpeername& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "getpeername request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_getsockname& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "getsockname request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_getsockopt& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "getsockopt request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_setsockopt& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "setsockopt request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_close& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "close request received\n");
                    return (handle_request_close(request));
                },
                [&](const api::request_connect& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "connect request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_listen& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "listen request received\n");
                    return (handle_request_generic(request));
                },
                [&](const api::request_socket& request) -> api::reply_msg {
                    OP_LOG(OP_LOG_TRACE, "socket request received\n");
                    return (handle_request_socket(request));
                }),
            request);

        if (send_reply(fd, client, reply) == -1) {
            OP_LOG(OP_LOG_ERROR, "Error sending reply on fd = %d: %s\n", fd,
                   strerror(errno));
        }
    }

    return (recv_or_err == 0 ? -1 : 0);
}

} // namespace openperf::socket::server
