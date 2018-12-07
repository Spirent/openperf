#include "memory/allocator/pool.h"
#include "socket/server_socket.h"
#include "core/icp_core.h"

namespace icp {
namespace sock {

server_socket::server_socket(api::socket_queue_pair* queues)
    : m_queues(queues)
{}

api::socket_queue_pair* server_socket::queues()
{
    return (m_queues);
}

int server_socket::client_fd()
{
    return (m_clientfd.get());
}

int server_socket::server_fd()
{
    return (m_serverfd.get());
}

int server_socket::process_reads()
{
    ICP_LOG(ICP_LOG_INFO, "Wake up for read on %d\n", m_serverfd.get());
    m_serverfd.read();
    return (0);
}

}
}
