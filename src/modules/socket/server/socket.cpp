#include "socket/server/socket.h"

namespace icp {
namespace socket {
namespace server {

socket::socket(icp::memory::allocator::pool& pool,
               int domain, int type, int protocol)
    : m_channel(new (pool.acquire()) io_channel(), io_channel_deleter(pool))
    , m_socket(type == SOCK_STREAM
               ? socket_variant{std::in_place_type<tcp_socket>}
               : socket_variant{std::in_place_type<udp_socket>})
{
    if (!m_channel) {
        throw std::runtime_error("Out of io_channels!");
    }
}

icp::socket::server::io_channel* socket::channel()
{
    return (m_channel.get());
}

api::reply_msg socket::handle_request(const api::request_msg& request)
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
