#ifndef _ICP_SOCKET_SERVER_API_HANDLER_H_
#define _ICP_SOCKET_SERVER_API_HANDLER_H_

#include <memory>
#include <unordered_map>

#include "socket/id_allocator.h"
#include "socket/server_socket.h"

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

namespace sock {

class server_api_handler {
    icp::core::event_loop& m_loop;
    icp::memory::allocator::pool& m_pool;

    static constexpr size_t max_sockets = 512;  /* XXX: need to make this dynamic */
    id_allocator<int, max_sockets> m_ids;

    std::unordered_map<int, std::shared_ptr<server_socket>> m_sockets;

    api::reply_msg handle_request_init(const api::request_init& request);
    api::reply_msg handle_request_accept(const api::request_accept& request);
    api::reply_msg handle_request_bind(const api::request_bind& request);
    api::reply_msg handle_request_shutdown(const api::request_shutdown& request);
    api::reply_msg handle_request_getpeername(const api::request_getpeername& request);
    api::reply_msg handle_request_getsockname(const api::request_getsockname& request);
    api::reply_msg handle_request_getsockopt(const api::request_getsockopt& request);
    api::reply_msg handle_request_setsockopt(const api::request_setsockopt& request);
    api::reply_msg handle_request_close(const api::request_close& request);
    api::reply_msg handle_request_connect(const api::request_connect& request);
    api::reply_msg handle_request_listen(const api::request_listen& request);
    api::reply_msg handle_request_socket(const api::request_socket& request);

public:
    server_api_handler(icp::core::event_loop& loop, icp::memory::allocator::pool& pool);

    int handle_requests(const struct icp_event_data* data);
};

}
}
#endif /* _ICP_SOCKET_SERVER_API_HANDLER_H_ */
