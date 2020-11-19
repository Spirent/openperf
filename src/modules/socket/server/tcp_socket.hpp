#ifndef _OP_SOCKET_SERVER_TCP_SOCKET_HPP_
#define _OP_SOCKET_SERVER_TCP_SOCKET_HPP_

#include <cerrno>
#include <list>
#include <queue>

#include "socket/server/allocator.hpp"
#include "socket/server/generic_socket.hpp"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/pbuf_queue.hpp"
#include "socket/server/socket_utils.hpp"
#include "socket/server/stream_channel.hpp"

struct pbuf;
struct tcp_pcb;

namespace openperf::socket::server {

struct tcp_init
{};
struct tcp_bound
{};
struct tcp_accept
{};
struct tcp_listening
{};
struct tcp_connecting
{};
struct tcp_connected
{};
struct tcp_closing
{};
struct tcp_closed
{};
struct tcp_error
{
    int value;
};

typedef std::variant<tcp_init,
                     tcp_bound,
                     tcp_accept,
                     tcp_listening,
                     tcp_connecting,
                     tcp_connected,
                     tcp_closing,
                     tcp_closed,
                     tcp_error>
    tcp_socket_state;

class tcp_socket : public socket_state_machine<tcp_socket, tcp_socket_state>
{
    struct tcp_pcb_deleter
    {
        void operator()(tcp_pcb* pcb);
    };
    typedef std::unique_ptr<tcp_pcb, tcp_pcb_deleter> tcp_pcb_ptr;

    stream_channel_ptr m_channel;
    tcp_pcb_ptr m_pcb;
    std::queue<tcp_pcb*, std::list<tcp_pcb*>> m_acceptq;
    pbuf_queue m_recvq;

    openperf::socket::server::allocator* channel_allocator();

    tcp_socket(openperf::socket::server::allocator* allocator,
               int flags,
               tcp_pcb* pcb);

public:
    tcp_socket(openperf::socket::server::allocator& allocator,
               enum lwip_ip_addr_type ip_type,
               int flags);
    ~tcp_socket();

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&&) = delete;

    tcp_socket& operator=(tcp_socket&& other) noexcept;
    tcp_socket(tcp_socket&& other) noexcept;

    /***
     * lwIP callback functions
     ***/
    int do_lwip_accept(tcp_pcb* newpcb, int err);
    int do_lwip_sent(uint16_t size);
    int do_lwip_recv(pbuf* p, int err);
    int do_lwip_connected(int err);
    int do_lwip_poll();
    int do_lwip_error(int err);

    /***
     * Generic socket functions
     ***/
    channel_variant channel() const;

    tl::expected<generic_socket, int> handle_accept(int);

    void handle_io();

    /***
     * Socket state machine functions
     ***/

    /* bind handlers */
    on_request_reply on_request(const api::request_bind&, const tcp_init&);

    /* listen handlers */
    on_request_reply on_request(const api::request_listen&, const tcp_bound&);
    on_request_reply on_request(const api::request_listen&, const tcp_error&);

    /* connection handlers */
    on_request_reply on_request(const api::request_connect&, const tcp_init&);
    on_request_reply on_request(const api::request_connect&, const tcp_bound&);
    on_request_reply on_request(const api::request_connect&,
                                const tcp_connecting&);
    on_request_reply on_request(const api::request_connect&,
                                const tcp_connected&);
    on_request_reply on_request(const api::request_connect&, const tcp_error&);

    /* shutdown handlers */
    on_request_reply on_request(const api::request_shutdown&,
                                const tcp_connected&);
    on_request_reply on_request(const api::request_shutdown&, const tcp_error&);

    template <typename State>
    on_request_reply on_request(const api::request_shutdown&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&,
                                const tcp_connected&);
    on_request_reply on_request(const api::request_getpeername&,
                                const tcp_error&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&,
                                const tcp_error&);

    static tl::expected<socklen_t, int>
    do_tcp_getsockname(const tcp_pcb*, const api::request_getsockname&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockname& name,
                                const State&)
    {
        auto result = do_tcp_getsockname(m_pcb.get(), name);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* getsockopt handlers */
    static tl::expected<socklen_t, int>
    do_getsockopt(const tcp_pcb*,
                  const api::request_getsockopt&,
                  const tcp_socket_state& state);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& name,
                                const State&)
    {
        auto result = do_getsockopt(m_pcb.get(), name, state());
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    on_request_reply on_request(const api::request_setsockopt&,
                                const tcp_error& error);

    static tl::expected<void, int>
    do_setsockopt(tcp_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& name,
                                const State&)
    {
        auto result = do_setsockopt(m_pcb.get(), name);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* ioctl handler */
    template <typename State>
    on_request_reply on_request(const api::request_ioctl& ioctl, const State&)
    {
        auto result =
            do_sock_ioctl(reinterpret_cast<ip_pcb*>(m_pcb.get()), ioctl);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* Generic handlers */
    template <typename Request, typename State>
    on_request_reply on_request(const Request&, const State&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }
};

const char* to_string(const tcp_socket_state&);

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_TCP_SOCKET_HPP_ */
