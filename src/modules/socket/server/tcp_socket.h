#ifndef _ICP_SOCKET_SERVER_TCP_SOCKET_H_
#define _ICP_SOCKET_SERVER_TCP_SOCKET_H_

#include <cerrno>
#include <list>
#include <queue>
//#include <memory>
//#include "tl/expected.hpp"

#include "socket/server/generic_socket.h"
#include "socket/server/socket_utils.h"
#include "socket/server/stream_channel.h"

struct tcp_pcb;

namespace icp {
namespace socket {
namespace server {


struct tcp_init {};
struct tcp_bound {};
struct tcp_accept {};
struct tcp_listening {};
struct tcp_connecting {};
struct tcp_connected {};
struct tcp_closing {};
struct tcp_closed {};
struct tcp_error{ int error; };

typedef std::variant<tcp_init,
                     tcp_bound,
                     tcp_accept,
                     tcp_listening,
                     tcp_connecting,
                     tcp_connected,
                     tcp_closing,
                     tcp_closed,
                     tcp_error> tcp_socket_state;

class tcp_socket : public socket_state_machine<tcp_socket, tcp_socket_state>
{
    struct tcp_pcb_deleter {
        void operator()(tcp_pcb* pcb);
    };
    typedef std::unique_ptr<tcp_pcb, tcp_pcb_deleter> tcp_pcb_ptr;

    stream_channel_ptr m_channel;
    tcp_pcb_ptr m_pcb;
    pid_t m_pid;
    std::queue<tcp_socket, std::list<tcp_socket>> m_acceptq;

    icp::memory::allocator::pool* channel_pool();

    tcp_socket(icp::memory::allocator::pool* pool, pid_t pid, tcp_pcb* pcb);

public:
    tcp_socket(icp::memory::allocator::pool& pool, pid_t pid);
    ~tcp_socket() = default;

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&&) = delete;

    tcp_socket& operator=(tcp_socket&& other) noexcept;
    tcp_socket(tcp_socket&& other) noexcept;

    /***
     * lwIP callback functions
     ***/
    int do_lwip_accept(tcp_pcb* newpcb, int err);
    int do_lwip_sent(uint16_t size);
    int do_lwip_recv(pbuf *p, int err);
    int do_lwip_connected(int err);
    int do_lwip_poll();
    int do_lwip_error(int err);

    /***
     * Generic socket functions
     ***/
    channel_variant channel() const;

    tl::expected<generic_socket, int> handle_accept();

    void handle_transmit();

    /***
     * Socket state machine functions
     ***/

    /* bind handlers */
    on_request_reply on_request(const api::request_bind&, const tcp_init&);

    /* listen handlers */
    on_request_reply on_request(const api::request_listen&, const tcp_bound&);

    /* connection handlers */
    on_request_reply on_request(const api::request_connect&, const tcp_init&);
    on_request_reply on_request(const api::request_connect&, const tcp_bound&);
    on_request_reply on_request(const api::request_connect&, const tcp_connecting&);
    on_request_reply on_request(const api::request_connect&, const tcp_connected&);

    /* shutdown handlers */
    on_request_reply on_request(const api::request_shutdown&, const tcp_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_shutdown&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&, const tcp_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    static tl::expected<socklen_t, int> do_tcp_getsockname(tcp_pcb*, const api::request_getsockname&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockname& name, const State&)
    {
        auto result = do_tcp_getsockname(m_pcb.get(), name);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* getsockopt handlers */
    static tl::expected<socklen_t, int> do_tcp_getsockopt(tcp_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& name, const State&)
    {
        auto result = do_tcp_getsockopt(m_pcb.get(), name);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    static tl::expected<void, int> do_tcp_setsockopt(tcp_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& name, const State&)
    {
        auto result = do_tcp_setsockopt(m_pcb.get(), name);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* Generic handler */
    template <typename State, typename Request>
    on_request_reply on_request(State&, const Request&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }
};

const char * to_string(const tcp_socket_state&);

}
}
}

#endif /* _ICP_SOCKET_SERVER_TCP_SOCKET_H_ */
