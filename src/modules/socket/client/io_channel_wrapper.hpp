#ifndef _OP_SOCKET_CLIENT_IO_CHANNEL_WRAPPER_HPP_
#define _OP_SOCKET_CLIENT_IO_CHANNEL_WRAPPER_HPP_

#include "tl/expected.hpp"

#include "socket/api.hpp"
#include "socket/client/dgram_channel.hpp"
#include "socket/client/stream_channel.hpp"

namespace openperf {
namespace socket {
namespace client {

class io_channel_wrapper
{
    using io_channel = std::variant<openperf::socket::client::dgram_channel*,
                                    openperf::socket::client::stream_channel*>;
    io_channel m_channel;

public:
    io_channel_wrapper(api::io_channel_ptr channel,
                       int client_fd,
                       int server_fd);
    ~io_channel_wrapper();

    io_channel_wrapper(const io_channel_wrapper&) = delete;
    io_channel_wrapper& operator=(const io_channel_wrapper&&) = delete;

    io_channel_wrapper(io_channel_wrapper&&) noexcept;
    io_channel_wrapper& operator=(io_channel_wrapper&&) noexcept;

    int error() const;

    int flags() const;
    int flags(int);

    tl::expected<size_t, int>
    send(const iovec iov[], size_t iovcnt, int flags, const sockaddr* to);

    tl::expected<size_t, int> recv(iovec iov[],
                                   size_t iovcnt,
                                   int flags,
                                   sockaddr* from,
                                   socklen_t* fromlen);

    tl::expected<void, int> block_writes();
    tl::expected<void, int> wait_readable();
    tl::expected<void, int> wait_writable();
};

} // namespace client
} // namespace socket
} // namespace openperf

#endif /* _OP_SOCKET_CLIENT_IO_CHANNEL_WRAPPER_HPP_ */
