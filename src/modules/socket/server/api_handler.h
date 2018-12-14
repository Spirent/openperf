#ifndef _ICP_SOCKET_SERVER_API_HANDLER_H_
#define _ICP_SOCKET_SERVER_API_HANDLER_H_

#include <cerrno>
#include <memory>
#include <unordered_map>

#include "tl/expected.hpp"

#include "socket/api.h"

struct icp_event_data;

namespace icp {

namespace core {
class event_loop;
}

namespace memory {
namespace allocator {
class pool;
}
}

namespace socket {
namespace server {

class socket;

class api_handler {
    icp::core::event_loop& m_loop;         /* event loop */
    icp::memory::allocator::pool& m_pool;  /* io_channel mempool */
    pid_t m_pid;                           /* client pid */
    uint32_t m_next_socket_id;             /* socket id counter */

    /* sockets by id */
    std::unordered_map<api::socket_id,
                       std::shared_ptr<server::socket>> m_sockets;

    api::reply_msg handle_request_init(const api::request_init& request);
    api::reply_msg handle_request_accept(const api::request_accept& request);
    api::reply_msg handle_request_close(const api::request_close& request);
    api::reply_msg handle_request_socket(const api::request_socket& request);

    template <typename Request>
    api::reply_msg handle_request_generic(const Request& request)
    {
        auto result = m_sockets.find(request.id);
        if (result == m_sockets.end()) {
            return (tl::make_unexpected(EINVAL));
        }

        auto& socket = result->second;
        return (socket->handle_request(request));
    }

public:
    api_handler(icp::core::event_loop& loop, icp::memory::allocator::pool& pool, pid_t pid);

    int handle_requests(const struct icp_event_data* data);
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_API_HANDLER_H_ */
