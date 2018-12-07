#ifndef _ICP_SOCKET_SERVER_H_
#define _ICP_SOCKET_SERVER_H_

#include <bitset>
#include <memory>
#include <unordered_map>

#include "core/icp_core.h"
#include "socket/shared_segment.h"
#include "socket/unix_socket.h"

struct icp_event_data;

namespace icp {

namespace memory {
namespace allocator {
class pool;
}
}

namespace sock {

class server_api_handler;

class server
{
    unix_socket m_sock;
    icp::memory::shared_segment m_shm;
    icp::core::event_loop& m_loop;
    std::unordered_map<pid_t, std::shared_ptr<server_api_handler>> m_handlers;
    icp::memory::allocator::pool* pool() const;

public:
    server(icp::core::event_loop& loop);

    int handle_accept_requests();
    int handle_init_requests(const struct icp_event_data*);
};

}
}
#endif /* _ICP_SOCKET_SERVER_H_ */
