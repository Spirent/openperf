#ifndef _ICP_SOCKET_API_SERVER_H_
#define _ICP_SOCKET_API_SERVER_H_

#include <bitset>
#include <memory>
#include <unordered_map>

#include "core/icp_core.h"
#include "socket/shared_segment.h"
#include "socket/unix_socket.h"
#include "socket/server/allocator.h"

struct icp_event_data;

namespace icp {
namespace memory {
namespace allocator {
class pool;
}
}
}

namespace icp {
namespace socket {
namespace server {
class api_handler;
}
namespace api {

class server
{
    static constexpr size_t shm_size = (1024 * 1024 * 1024);  /* 1 GB */

    unix_socket m_sock;
    icp::memory::shared_segment m_shm;
    icp::core::event_loop& m_loop;
    std::unordered_map<pid_t, std::unique_ptr<socket::server::api_handler>> m_handlers;
    std::unordered_multimap<int, pid_t> m_pids;
    socket::server::allocator* allocator() const;

public:
    server(icp::core::event_loop& loop);
    ~server();  /* Explicit destructor needed to avoid pulling in api_handler header */

    int handle_api_accept_requests();
    int handle_api_init_requests(const struct icp_event_data*);
    int handle_api_read_requests(const struct icp_event_data*);
    int handle_api_delete_requests(const struct icp_event_data*);
};

}
}
}

#endif /* _ICP_SOCKET_API_SERVER_H_ */
