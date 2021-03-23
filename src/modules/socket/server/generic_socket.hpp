#ifndef _OP_SOCKET_SERVER_GENERIC_SOCKET_HPP_
#define _OP_SOCKET_SERVER_GENERIC_SOCKET_HPP_

#include <memory>
#include "tl/expected.hpp"

#include "socket/api.hpp"
#include "socket/server/allocator.hpp"

namespace openperf::socket::server {

class dgram_channel;
class stream_channel;

typedef std::variant<dgram_channel*, stream_channel*> channel_variant;

/**
 * A type erasing definition of a socket.
 */
class generic_socket
{
public:
    template <typename Socket>
    generic_socket(Socket socket)
        : m_self(std::make_shared<socket_model<Socket>>(std::move(socket)))
    {}

    channel_variant channel() const { return (m_self->channel()); }

    api::reply_msg handle_request(const api::request_msg& request)
    {
        return (m_self->handle_request(request));
    }

    tl::expected<generic_socket, int> handle_accept(int flags)
    {
        return (m_self->handle_accept(flags));
    }

    void handle_io() { m_self->handle_io(); }

    void* pcb() const { return m_self->pcb(); }

private:
    struct socket_concept
    {
        virtual ~socket_concept() = default;
        virtual channel_variant channel() const = 0;
        virtual api::reply_msg handle_request(const api::request_msg&) = 0;
        virtual tl::expected<generic_socket, int> handle_accept(int) = 0;
        virtual void handle_io() = 0;
        virtual void* pcb() const = 0;
    };

    template <typename Socket> struct socket_model final : socket_concept
    {
        socket_model(Socket socket)
            : m_socket(std::move(socket))
        {}

        channel_variant channel() const override
        {
            return (m_socket.channel());
        }

        api::reply_msg handle_request(const api::request_msg& request) override
        {
            return (m_socket.handle_request(request));
        }

        tl::expected<generic_socket, int> handle_accept(int flags) override
        {
            return (m_socket.handle_accept(flags));
        }

        void handle_io() override { m_socket.handle_io(); }

        void* pcb() const override
        {
            return static_cast<void*>(m_socket.pcb());
        }

        Socket m_socket;
    };

    std::shared_ptr<socket_concept> m_self;
};

tl::expected<generic_socket, int>
make_socket(allocator& allocator, int domain, int type, int protocol);

api::io_channel_offset api_channel_offset(channel_variant&, const void* base);
int client_fd(channel_variant&);
int server_fd(channel_variant&);

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_GENERIC_SOCKET_HPP_ */
