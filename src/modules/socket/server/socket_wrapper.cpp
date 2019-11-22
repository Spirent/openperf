#include "socket/server/socket_wrapper.h"

namespace openperf {
namespace socket {
namespace server {

static socket make_socket(int type, int protocol)
{
    socket to_return;
    switch (type) {
    case SOCK_STREAM:
        return (tcp_socket());
    case SOCK_DGRAM:
        return (udp_socket());
    case SOCK_RAW:
        return (raw_socket());
    default:
        return (std::monostate);
    }
}

static io_channel* acquire_io_channel(openperf::memory::allocator::pool& pool)
{
    return (reinterpret_cast<io_channel*>(pool.acquire()));
}

socket_wrapper::socket_wrapper(size_t id,
                               openperf::memory::allocator::pool& pool,
                               int domain, int type, int protocol)
    : m_channel(acquire_io_channel(pool), io_channel_deleter(pool))
    , m_socket(make_socket(type, protocol))
    , m_id(id)
{}

size_t socket_wrapper::id() { return (m_id); }

api::reply_msg socket_wrapper::handle_request(const api::request_msg& request)
{
    auto request_visitor = [&](auto& socket) -> api::reply_msg {
        return (socket.handle_request(request));
    };
    return (std::visit(request_visitor, m_socket));
}

} // namespace server
} // namespace socket
} // namespace openperf
