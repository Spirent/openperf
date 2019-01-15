#ifndef _ICP_SOCKET_SERVER_GENERIC_SOCKET_H_
#define _ICP_SOCKET_SERVER_GENERIC_SOCKET_H_

#include <memory>
#include "tl/expected.hpp"

#include "memory/allocator/pool.h"
#include "socket/api.h"

namespace icp {
namespace socket {
namespace server {

class dgram_channel;
class stream_channel;

typedef std::variant<dgram_channel*,
                     stream_channel*> channel_variant;

/**
 * A type erasing definition of a socket.
 */
class generic_socket {
public:
    template <typename Socket>
    generic_socket(Socket socket)
        : m_self(std::make_shared<socket_model<Socket>>(std::move(socket)))
    {}

    channel_variant channel() const
    {
        return (m_self->channel());
    }

    api::reply_msg handle_request(const api::request_msg& request)
    {
        return (m_self->handle_request(request));
    }

    tl::expected<generic_socket, int> handle_accept(int flags)
    {
        return (m_self->handle_accept(flags));
    }

    void handle_transmit()
    {
        m_self->handle_transmit();
    }

private:
    struct socket_concept {
        virtual ~socket_concept() = default;
        virtual channel_variant channel() const = 0;
        virtual api::reply_msg handle_request(const api::request_msg&) = 0;
        virtual tl::expected<generic_socket, int> handle_accept(int) = 0;
        virtual void handle_transmit() = 0;
    };

    template <typename Socket>
    struct socket_model final : socket_concept {
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

        void handle_transmit() override
        {
            m_socket.handle_transmit();
        }

        Socket m_socket;
    };

    std::shared_ptr<socket_concept> m_self;
};

tl::expected<generic_socket,int> make_socket(icp::memory::allocator::pool& pool, pid_t pid,
                                             int domain, int type, int protocol);

api::io_channel api_channel(channel_variant&);
int client_fd(channel_variant&);
int server_fd(channel_variant&);

}
}
}

#endif /* _ICP_SOCKET_SERVER_GENERIC_SOCKET_H_ */
