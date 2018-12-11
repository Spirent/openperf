#include <sys/eventfd.h>

#include "socket/server_socket.h"
#include "core/icp_core.h"

namespace icp {
namespace sock {

server_socket::server_socket(int sockid,
                             api::socket_queue_pair* queues,
                             std::function<void()> on_delete)
    : m_deleter(std::move(on_delete))
    , m_queues(queues)
    , m_clientfd(eventfd(0, 0))
    , m_serverfd(eventfd(0, 0))
    , m_sockid(sockid)
    , m_recvq(&m_queues->client_q, m_clientfd, m_serverfd)
    , m_sendq(&m_queues->server_q, m_serverfd, m_clientfd)
{
    if (m_serverfd == -1 || m_clientfd == -1) {
        throw std::runtime_error("Could not create one or more eventfds: "
                                 + std::string(strerror(errno)));
    }
}

server_socket::~server_socket()
{
    close(m_clientfd);
    close(m_serverfd);
    m_deleter();
}

api::socket_queue* server_socket::client_q()
{
    return (&m_queues->client_q);
}

api::socket_queue* server_socket::server_q()
{
    return (&m_queues->server_q);
}

int server_socket::client_fd()
{
    return (m_clientfd);
}

int server_socket::server_fd()
{
    return (m_serverfd);
}

int server_socket::id()
{
    return (m_sockid);
}

}
}
