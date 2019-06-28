#ifndef _ICP_SOCKET_API_SERVER_H_
#define _ICP_SOCKET_API_SERVER_H_

#include <memory>
#include <unordered_map>

#include "core/icp_core.h"
#include "packetio/internal_client.h"
#include "packetio/generic_event_loop.h"
#include "socket/shared_segment.h"
#include "socket/unix_socket.h"
#include "socket/server/allocator.h"

namespace icp::memory::allocator {
class pool;
}

namespace icp::socket::server {
class api_handler;
}

namespace icp::socket::api {

class server
{
    static constexpr size_t shm_size = (1024 * 1024 * 1024);  /* 1 GB */

    void* m_context;
    unix_socket m_sock;
    icp::memory::shared_segment m_shm;
    std::unordered_map<pid_t, std::unique_ptr<socket::server::api_handler>> m_handlers;
    std::unordered_multimap<int, pid_t> m_pids;
    std::string m_task;

    socket::server::allocator* allocator() const;

    using event_loop = icp::packetio::event_loop::generic_event_loop;

    int handle_api_accept(event_loop& loop, std::any);
    int handle_api_delete(int fd);
    int handle_api_init(event_loop& loop, std::any arg);
    int handle_api_read(event_loop& loop, std::any arg);

public:
    server(void* context);
    ~server();  /* Explicit destructor needed to avoid pulling in api_handler header */

    int start();
    void stop();
};

}

#endif /* _ICP_SOCKET_API_SERVER_H_ */
