#ifndef _ICP_SOCKET_SERVER_SOCKET_H_
#define _ICP_SOCKET_SERVER_SOCKET_H_

#include "memory/allocator/pool.h"
#include "socket/socket_api.h"
#include "socket/eventfd_queue.h"

namespace icp {
namespace sock {

class server_socket {
    std::function<void()> m_deleter;
    api::socket_queue_pair* m_queues;
    int m_clientfd;
    int m_serverfd;
    int m_sockid;

    eventfd_sender<iovec, api::socket_queue_length>   m_recvq;  /* from stack to client */
    eventfd_receiver<iovec, api::socket_queue_length> m_sendq;  /* from client to stack */

public:
    server_socket(int sockid, api::socket_queue_pair* queues, std::function<void()> on_delete);
    ~server_socket();

    server_socket(const server_socket&) = delete;
    server_socket& operator=(const server_socket&) = delete;

    api::socket_queue* client_q();
    api::socket_queue* server_q();
    int client_fd();
    int server_fd();
    int id();

    void read() { m_recvq.wait(); }
};

}
}

#endif /* _ICP_SOCKET_SERVER_SOCKET_H_ */
