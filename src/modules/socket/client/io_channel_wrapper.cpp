#include "socket/client/io_channel_wrapper.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf {
namespace socket {
namespace client {

using io_channel = std::variant<dgram_channel*, stream_channel*>;

static io_channel io_channel_cast(api::io_channel_ptr channel)
{
    return (std::visit(utils::overloaded_visitor(
                           [](socket::dgram_channel* channel) -> io_channel {
                               return (reinterpret_cast<dgram_channel*>(channel));
                           },
                           [](socket::stream_channel* channel) -> io_channel {
                               return (reinterpret_cast<stream_channel*>(channel));
                           }),
                       channel));
}

io_channel_wrapper::io_channel_wrapper(api::io_channel_ptr channel, int client_fd, int server_fd)
    : m_channel(io_channel_cast(channel))
{
    std::visit(utils::overloaded_visitor(
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
    std::visit(utils::overloaded_visitor(
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

int io_channel_wrapper::error() const
{
    auto error_visitor = [](auto channel) -> int {
                             return (channel->error());
                         };
    return (std::visit(error_visitor, m_channel));
}

int io_channel_wrapper::flags() const
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

tl::expected<size_t, int> io_channel_wrapper::send(const iovec iov[], size_t iovcnt,
                                                   int flags,
                                                   const sockaddr *to)
{
    auto send_visitor = [&](auto channel) -> tl::expected<size_t, int> {
        return (channel->send(iov, iovcnt, flags, to));
    };
    return (std::visit(send_visitor, m_channel));
}

tl::expected<size_t, int> io_channel_wrapper::recv(iovec iov[], size_t iovcnt,
                                                   int flags,
                                                   sockaddr *from, socklen_t *fromlen)
{
    auto recv_visitor = [&](auto channel) -> tl::expected<size_t, int> {
        return (channel->recv(iov, iovcnt, flags, from, fromlen));
    };
    return (std::visit(recv_visitor, m_channel));
}

tl::expected<void, int> io_channel_wrapper::block_writes()
{
    auto block_writes_visitor = [&](auto channel) -> tl::expected<void, int> {
        return (channel->block_writes());
    };
    return (std::visit(block_writes_visitor, m_channel));
}

tl::expected<void, int> io_channel_wrapper::wait_readable()
{
    auto wait_readable_visitor = [&](auto channel) -> tl::expected<void, int> {
        return (channel->wait_readable());
    };
    return (std::visit(wait_readable_visitor, m_channel));
}

tl::expected<void, int> io_channel_wrapper::wait_writable()
{
    auto wait_writable_visitor = [&](auto channel) -> tl::expected<void, int> {
        return (channel->wait_writable());
    };
    return (std::visit(wait_writable_visitor, m_channel));
}

}
}
}
