#ifndef _ICP_SOCKET_SERVER_SOCKET_H_
#define _ICP_SOCKET_SERVER_SOCKET_H_

#include <memory>
#include <variant>

#include "memory/allocator/pool.h"
#include "socket/api.h"
#include "socket/server/io_channel.h"
#include "socket/server/tcp_socket.h"
#include "socket/server/udp_socket.h"

namespace icp {
namespace socket {
namespace server {

typedef std::variant<udp_socket,
                     tcp_socket> socket_variant;

class socket
{
    struct io_channel_deleter {
        using pool = icp::memory::allocator::pool;
        pool& pool_;
        void operator()(icp::socket::server::io_channel *c) {
            c->~io_channel();
            pool_.release(reinterpret_cast<void*>(c));
        }
        io_channel_deleter(pool& p)
            : pool_(p)
        {}
    };

    typedef std::unique_ptr<icp::socket::server::io_channel, io_channel_deleter> channel_ptr;
    channel_ptr m_channel;
    socket_variant m_socket;

public:
    socket(icp::memory::allocator::pool& pool,
           int domain, int type, int protocol);

    socket(const socket&) = delete;
    socket& operator=(const socket&&) = delete;

    icp::socket::server::io_channel* channel();

    api::reply_msg handle_request(const api::request_msg& request);
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_SOCKET_H_ */
