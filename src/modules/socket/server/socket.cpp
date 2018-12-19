#include "socket/server/socket.h"

namespace icp {
namespace socket {
namespace server {

socket::socket(icp::memory::allocator::pool& pool, pid_t pid,
               int domain, int type, int protocol)
    : m_channel(new (pool.acquire()) io_channel(), io_channel_deleter(pool))
    , m_socket(type == SOCK_STREAM
               ? socket_variant{std::in_place_type<tcp_socket>, m_channel.get()}
               : socket_variant{std::in_place_type<udp_socket>, m_channel.get()})
    , m_pid(pid)
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

void socket::handle_transmit()
{
    m_channel->recv_ack();
    std::visit(overloaded_visitor(
                   [&](tcp_socket& socket) {
                       socket.handle_transmit(m_pid, m_channel.get());
                   },
                   [&](udp_socket& socket) {
                       socket.handle_transmit(m_pid, m_channel.get());
                   }),
               m_socket);
    m_channel->garbage_collect();
}

}
}
}
