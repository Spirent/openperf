#include "socket/server/socket_wrapper.h"

namespace icp {
namespace socket {
namespace server {

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static
socket make_socket(int type, int protocol)
{
    socket to_return;
    switch (type) {
    case SOCK_STREAM:
        return (tcp_socket());
    case SOCK_DGRAM:
        return (udp_socket());
    default:
        return (std::monostate);
    }
}

static
io_channel* acquire_io_channel(icp::memory::allocator::pool& pool)
{
    return (reinterpret_cast<io_channel*>(pool.acquire()));
}

socket_wrapper::socket_wrapper(size_t id, icp::memory::allocator::pool& pool,
                               int domain, int type, int protocol)
    : m_channel(acquire_io_channel(pool), io_channel_deleter(pool))
    , m_socket(make_socket(type, protocol))
    , m_id(id)
{}

size_t socket_wrapper::id() { return (m_id); }

api::reply_msg socket_wrapper::handle_request(const api::request_msg& request)
{
    return (std::visit(overloaded_visitor(
                           [&](tcp_socket& socket) -> api::reply_msg {
                               return (socket.handle_request(request));
                           },
                           [&](udp_socket& socket) -> api::reply_msg {
                               return (socket.handle_request(request));
                           }),
                       m_socket));
}

}
}
}
