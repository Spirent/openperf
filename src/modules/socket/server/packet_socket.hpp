#ifndef _OP_SOCKET_SERVER_PACKET_SOCKET_HPP_
#define _OP_SOCKET_SERVER_PACKET_SOCKET_HPP_

#include <variant>

#include "tl/expected.hpp"

#include "socket/server/dgram_channel.hpp"
#include "socket/server/generic_socket.hpp"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/socket_utils.hpp"

struct packet_pcb;
struct pbuf;

namespace openperf::socket::server {

struct packet_init
{};
struct packet_bound
{};
struct packet_closed
{};

using packet_socket_state =
    std::variant<packet_init, packet_bound, packet_closed>;

class packet_socket
    : public socket_state_machine<packet_socket, packet_socket_state>
{
    dgram_channel_ptr m_channel; /* shared memory io channel */
    packet_pcb* m_pcb;           /* lwIP pcb */

public:
    packet_socket(openperf::socket::server::allocator& allocator,
                  int type,
                  int protocol);
    ~packet_socket();

    packet_socket(const packet_socket&) = delete;
    packet_socket& operator=(const packet_socket&&) = delete;

    packet_socket& operator=(packet_socket&& other) noexcept;
    packet_socket(packet_socket&& other) noexcept;

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
    on_request_reply on_request(const api::request_bind&, const packet_init&);

    /* getpeername handler */
    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&,
                                const packet_bound&);

    /* getsockopt handlers */
    static tl::expected<socklen_t, int>
    do_getsockopt(const packet_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& opt,
                                const State&)
    {
        auto result = do_getsockopt(m_pcb, opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    static tl::expected<void, int>
    do_setsockopt(packet_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& opt,
                                const State&)
    {
        auto result = do_setsockopt(m_pcb, opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* ioctl handler */
    template <typename State>
    on_request_reply on_request(const api::request_ioctl& ioctl, const State&)
    {
        auto result = do_sock_ioctl(reinterpret_cast<ip_pcb*>(m_pcb), ioctl);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* generic handler */
    template <typename Request, typename State>
    on_request_reply on_request(const Request&, const State&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }
};

const char* to_string(const packet_socket_state&);

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_PACKET_SOCKET_HPP_ */
