#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "socket/socket_api.h"
#include "socket/client.h"

namespace icp {
namespace sock {

client::client()
    : m_uuid(uuid::random())
    , m_sock(api::client_socket(to_string(m_uuid)), api::socket_type)
{
    auto server = sockaddr_un{ .sun_family = AF_UNIX };
    std::strncpy(server.sun_path, api::server_socket().c_str(),
                 sizeof(server.sun_path));

    if (connect(m_sock.get(),
                reinterpret_cast<sockaddr*>(&server),
                sizeof(server)) == -1) {
        throw std::runtime_error("Could not connect to socket server: "
                                 + std::string(strerror(errno)));
    }
}


static api::reply_msg submit_request(int sockfd,
                                     const api::request_msg& request)
{
    if (send(sockfd,
             const_cast<void*>(reinterpret_cast<const void*>(&request)),
             sizeof(request),
             0) == -1) {
        return (tl::make_unexpected(errno));
    }

    /* Setup to receive a message */
    api::reply_msg reply;

    struct iovec iov = {
        .iov_base = reinterpret_cast<void*>(&reply),
        .iov_len = sizeof(reply)
    };

    /* Create a properly aligned data buffer for our cmsg data */
    union {
        char data[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } control;

    struct msghdr msg = {
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control.data,
        .msg_controllen = sizeof(control.data)
    };

    if (recvmsg(sockfd, &msg, 0) == -1) {
        return (tl::make_unexpected(errno));
    }

    if (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != nullptr) {
        if (cmsg->cmsg_len == CMSG_LEN(sizeof(api::socket_fd_pair))
            && cmsg->cmsg_level == SOL_SOCKET
            && cmsg->cmsg_type == SCM_RIGHTS) {
            /*
             * Message contains a fd, update the message sockfd value so the
             * client can use it directly.
             */
            set_message_fds(reply, *(reinterpret_cast<api::socket_fd_pair*>(CMSG_DATA(cmsg))));
        }
    }

    return (reply);
}

/* Send a hello message to server; wait for reply */
void client::init()
{
    api::request_msg request = api::request_init{
        .pid = getpid(),
        .tid = m_uuid
    };

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        printf("error: %s\n", strerror(reply.error()));
        return;
    }

    auto init = std::get<api::reply_init>(*reply);
    if (init.shm_info) {
        /* map shared address memory */
        auto shm_info = *init.shm_info;
        m_shm.reset(new memory::shared_segment(shm_info.name,
                                               shm_info.size,
                                               shm_info.base));
    }
}

int client::socket(int domain, int type, int protocol)
{
    api::request_msg request = api::request_socket{
        .domain = domain,
        .type = type,
        .protocol = protocol
    };

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    auto s = std::get<api::reply_socket>(*reply);

    /* Record socket data for future use */
    auto result = m_sockets.emplace(s.fd_pair.client_fd,
                                    client_socket(s.sockid, s.client_q, s.fd_pair.client_fd,
                                                  s.server_q, s.fd_pair.server_fd));

    if (!result.second) {
        errno = ENOBUFS;
        return (-1);
    }

    /* Give the client their fd */
    return (s.fd_pair.client_fd);
}

int client::close(int s)
{
    auto result = m_sockets.find(s);
    if (result == m_sockets.end()) {
        /* XXX: should hand off to libc close */
        errno = EINVAL;
        return (-1);
    }

    auto& sock = result->second;
    api::request_msg request = api::request_close{
        .sockid = sock.id
    };

    auto reply = submit_request(m_sock.get(), request);
    if (!reply) {
        errno = reply.error();
        return (-1);
    }

    /* XXX: libc close */
    close(sock.client_fd());
    close(sock.server_fd());

    m_sockets.erase(result);
    return (0);
}

int client::io_ping(int s)
{
    auto result = m_sockets.find(s);
    if (result == m_sockets.end()) {
        errno = EINVAL;
        return (-1);
    }

    auto& sock = result->second;
    eventfd_write(sock.server_fd(), 1);
    return (0);
}
}
}
