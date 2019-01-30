#include "socket/client/io_channel_wrapper.h"

namespace icp {
namespace socket {
namespace client {

io_channel_wrapper::io_channel_wrapper(api::io_channel& channel, int client_fd, int server_fd)
    : m_channel(*reinterpret_cast<io_channel*>(&channel))
{
    std::visit(overloaded_visitor(
                   [&](dgram_channel* channel) {
                       new (channel) dgram_channel(client_fd, server_fd);
                   },
                   [&](stream_channel* channel) {
                       new (channel) stream_channel(client_fd, server_fd);
                   }),
               m_channel);
}

io_channel_wrapper::~io_channel_wrapper()
{
    std::visit(overloaded_visitor(
                   [](dgram_channel* channel) {
                       if (channel) channel->~dgram_channel();
                   },
                   [](stream_channel* channel) {
                       if (channel) channel->~stream_channel();
                   }),
               m_channel);
}

io_channel_wrapper::io_channel_wrapper(io_channel_wrapper&& other)
{
    std::swap(m_channel, other.m_channel);
}

io_channel_wrapper& io_channel_wrapper::operator=(io_channel_wrapper&& other)
{
    if (this != &other) {
        std::swap(m_channel, other.m_channel);
    }
    return (*this);
}

int io_channel_wrapper::flags()
{
    auto flags_visitor = [](auto channel) -> int {
                             return (channel->flags());
                         };
    return (std::visit(flags_visitor, m_channel));
}

int io_channel_wrapper::flags(int new_flags)
{
    auto flags_visitor = [&](auto channel) -> int {
                             return (channel->flags(new_flags));
                         };
    return (std::visit(flags_visitor, m_channel));
}

tl::expected<size_t, int> io_channel_wrapper::send(pid_t pid,
                                                   const iovec iov[], size_t iovcnt,
                                                   const sockaddr *to)
{
    auto send_visitor = [&](auto channel) -> tl::expected<size_t, int> {
        return (channel->send(pid, iov, iovcnt, to));
    };
    return (std::visit(send_visitor, m_channel));
}

tl::expected<size_t, int> io_channel_wrapper::recv(pid_t pid,
                                                   iovec iov[], size_t iovcnt,
                                                   sockaddr *from, socklen_t *fromlen)
{
    auto recv_visitor = [&](auto channel) -> tl::expected<size_t, int> {
        return (channel->recv(pid, iov, iovcnt, from, fromlen));
    };
    return (std::visit(recv_visitor, m_channel));
}

int io_channel_wrapper::recv_clear()
{
    auto recv_clear_visitor = [](auto channel) -> int {
                                  return (channel->recv_clear());
                              };
    return (std::visit(recv_clear_visitor, m_channel));
}

}
}
}
