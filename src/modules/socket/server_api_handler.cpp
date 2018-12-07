#include <cerrno>
#include <stdexcept>
#include <sys/un.h>

#include "memory/allocator/pool.h"
#include "socket/server_api_handler.h"
#include "core/icp_core.h"

namespace icp {
namespace sock {

server_api_handler::server_api_handler(icp::core::event_loop& loop,
                                       icp::memory::allocator::pool& pool)
    : m_loop(loop)
    , m_pool(pool)
{}

static
api::socket_queue_pair* acquire_queues(icp::memory::allocator::pool& pool)
{
    return (reinterpret_cast<api::socket_queue_pair*>(pool.acquire()));
}

static
void release_queues(icp::memory::allocator::pool& pool,
                    const api::socket_queue_pair* queues)
{
    pool.release(reinterpret_cast<const void*>(queues));
}

static ssize_t send_reply(int sockfd, const sockaddr_un& client, const api::reply_msg& reply)
{
    struct iovec iov = {
        .iov_base = const_cast<void*>(reinterpret_cast<const void*>(&reply)),
        .iov_len = sizeof(reply)
    };

    struct msghdr msg = {
        .msg_name = const_cast<void*>(reinterpret_cast<const void*>(&client)),
        .msg_namelen = sizeof(client),
        .msg_iov = &iov,
        .msg_iovlen = 1,
    };

    if (auto fd_pair = api::get_message_fds(reply)) {
        /* Create a properly aligned data buffer for our cmsg data */
        union {
            char data[CMSG_SPACE(sizeof(*fd_pair))];
            struct cmsghdr align;
        } control;

        msg.msg_control = control.data;
        msg.msg_controllen = sizeof(control.data);

        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(*fd_pair));
        memcpy(CMSG_DATA(cmsg), &(*fd_pair), sizeof(*fd_pair));

        ICP_LOG(ICP_LOG_INFO, "Sending client_fd = %d, server_fd = %d\n",
                fd_pair->client_fd, fd_pair->server_fd);
    }

    return (sendmsg(sockfd, &msg, 0));
}

static int handle_socket_read(const struct icp_event_data *data __attribute__((unused)),
                              void *arg)
{
    auto socket = *(reinterpret_cast<std::shared_ptr<server_socket>*>(arg));
    return (socket->process_reads());
}

static int handle_socket_delete(const struct icp_event_data *data __attribute__((unused)),
                                void *arg)
{
    auto socket = *(reinterpret_cast<std::shared_ptr<server_socket>*>(arg));
    assert(socket.use_count() > 0);
    socket.reset();
    return (0);
}

api::reply_msg server_api_handler::handle_request_accept(const api::request_accept& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_bind(const api::request_bind& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_shutdown(const api::request_shutdown& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_getpeername(const api::request_getpeername& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_getsockname(const api::request_getsockname& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_getsockopt(const api::request_getsockopt& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_setsockopt(const api::request_setsockopt& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_close(const api::request_close& request)
{
    /* lookup socket to close */
    auto result = m_sockets.find(request.sockid);
    if (result == m_sockets.end()) {
        return (tl::make_unexpected(EINVAL));
    }

    auto& s = result->second;
    m_loop.del(s->server_fd());
    m_ids.release(request.sockid);
    release_queues(m_pool, s->queues());
    m_sockets.erase(result);
    return (api::reply_success());
}

api::reply_msg server_api_handler::handle_request_connect(const api::request_connect& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_listen(const api::request_listen& request)
{
    return (tl::make_unexpected(ENOSYS));
}

api::reply_msg server_api_handler::handle_request_socket(const api::request_socket& request)
{
    auto id = m_ids.acquire();
    if (!id) {
        return (tl::make_unexpected(EMFILE));
    }

    auto queues = acquire_queues(m_pool);
    if (!queues) {
        m_ids.release(*id);
        return (tl::make_unexpected(ENOBUFS));
    }

    auto result = m_sockets.emplace(*id, std::make_shared<server_socket>(queues));
    if (!result.second) {
        m_ids.release(*id);
        release_queues(m_pool, queues);
        return (tl::make_unexpected(ENOMEM));
    }

    auto& s = result.first->second;

    /* Add I/O callback for socket writes from client to stack */
    struct icp_event_callbacks socket_callbacks = {
        .on_read   = handle_socket_read,
        .on_delete = handle_socket_delete
    };
    m_loop.add(s->server_fd(), &socket_callbacks,
               reinterpret_cast<void*>(new std::shared_ptr(s)));  /* bump ref count on ptr */

    return (api::reply_socket{
            .sockid = *id,
            .queue_pair = s->queues(),
            .fd_pair = {
                .client_fd = s->client_fd(),
                .server_fd = s->server_fd(),
            }
        });
}

int server_api_handler::handle_requests(const struct icp_event_data *data)
{
    api::request_msg request;
    ssize_t recv_or_err = 0;
    sockaddr_un client;
    socklen_t client_length = sizeof(client);
    while ((recv_or_err = recvfrom(data->fd,
                                   reinterpret_cast<void*>(&request), sizeof(request),
                                   0,
                                   reinterpret_cast<sockaddr*>(&client), &client_length))
           == sizeof(api::request_msg)) {

        auto reply = std::visit(overloaded_visitor(
                                    [](const api::request_init&) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "init request received\n");
                                        return (tl::make_unexpected(EINVAL));
                                    },
                                    [&](const api::request_accept& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "accept request received\n");
                                        return (handle_request_accept(request));
                                    },
                                    [&](const api::request_bind& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "bind request received\n");
                                        return (handle_request_bind(request));
                                    },
                                    [&](const api::request_shutdown& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "shutdown request received\n");
                                        return (handle_request_shutdown(request));
                                    },
                                    [&](const api::request_getpeername& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "getpeername request received\n");
                                        return (handle_request_getpeername(request));
                                    },
                                    [&](const api::request_getsockname& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "getsockname request received\n");
                                        return (handle_request_getsockname(request));
                                    },
                                    [&](const api::request_getsockopt& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "getsockopt request received\n");
                                        return (handle_request_getsockopt(request));
                                    },
                                    [&](const api::request_setsockopt& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "setsockopt request received\n");
                                        return (handle_request_setsockopt(request));
                                    },
                                    [&](const api::request_close& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "close request received\n");
                                        return (handle_request_close(request));
                                    },
                                    [&](const api::request_connect& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "connect request received\n");
                                        return (handle_request_connect(request));
                                    },
                                    [&](const api::request_listen& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "listen request received\n");
                                        return (handle_request_listen(request));
                                    },
                                    [&](const api::request_socket& request) -> api::reply_msg {
                                        ICP_LOG(ICP_LOG_TRACE, "socket request received\n");
                                        return (handle_request_socket(request));
                                    }),
                                request);

        if (send_reply(data->fd, client, reply) == -1) {
            ICP_LOG(ICP_LOG_ERROR, "Error sending reply on fd = %d: %s\n",
                    data->fd, strerror(errno));
        }
    }

    return (recv_or_err == 0 ? -1 : 0);
}

}
}
