#ifndef _ICP_SOCKET_SERVER_SOCKET_H_
#define _ICP_SOCKET_SERVER_SOCKET_H_

#include "socket/eventfd_wrapper.h"
#include "socket/socket_api.h"

namespace icp {
namespace sock {

class alignas(64) server_socket {
    api::socket_queue_pair* m_queues;
    eventfd_wrapper m_clientfd;
    eventfd_wrapper m_serverfd;

public:
    server_socket(api::socket_queue_pair* queues);

    server_socket(const server_socket&) = delete;
    server_socket& operator=(const server_socket&) = delete;

    api::socket_queue_pair* queues();
    int client_fd();
    int server_fd();

    int process_reads();
};

}
}
#endif /* _ICP_SOCKET_SERVER_SOCKET_H_ */
