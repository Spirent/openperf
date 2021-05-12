#ifndef _OP_SOCKET_SERVER_RAW_SOCKET_HPP_
#define _OP_SOCKET_SERVER_RAW_SOCKET_HPP_

#include <cerrno>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "tl/expected.hpp"

#include "socket/server/allocator.hpp"
#include "socket/server/dgram_channel.hpp"
#include "socket/server/generic_socket.hpp"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/socket_utils.hpp"

#include "lwip/raw.h"

struct raw_pcb;
struct pbuf;

namespace openperf::socket::server {

struct raw_init
{};
struct raw_bound
{};
struct raw_connected
{};
struct raw_closed
{};

typedef std::variant<raw_init, raw_bound, raw_connected, raw_closed>
    raw_socket_state;

class raw_socket : public socket_state_machine<raw_socket, raw_socket_state>
{
public:
    raw_socket(openperf::socket::server::allocator& allocator,
               enum lwip_ip_addr_type ip_type,
               int flags,
               int protocol,
               raw_recv_fn recv_callback = nullptr);
    ~raw_socket() = default;

    raw_socket(const raw_socket&) = delete;
    raw_socket& operator=(const raw_socket&&) = delete;

    raw_socket& operator=(raw_socket&& other) noexcept;
    raw_socket(raw_socket&& other) noexcept;

    int sock_type() const;

    /***
     * Generic socket functions
     ***/
    channel_variant channel() const;

    tl::expected<generic_socket, int> handle_accept(int);

    void handle_io();

    raw_pcb* pcb() const { return m_pcb.get(); }

    /***
     * Socket state machine functions
     ***/

    /* bind handlers */
    on_request_reply on_request(const api::request_bind&, const raw_init&);

    /* connect handlers */
    on_request_reply on_request(const api::request_connect&, const raw_init&);
    on_request_reply on_request(const api::request_connect&, const raw_bound&);
    on_request_reply on_request(const api::request_connect&,
                                const raw_connected&);

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&,
                                const raw_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&,
                                const raw_bound&);
    on_request_reply on_request(const api::request_getsockname&,
                                const raw_connected&);

    /* getsockopt handlers */
    virtual tl::expected<socklen_t, int>
    do_getsockopt(const raw_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& opt,
                                const State&)
    {
        auto result = do_getsockopt(m_pcb.get(), opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    virtual tl::expected<void, int>
    do_setsockopt(raw_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& opt,
                                const State&)
    {
        auto result = do_setsockopt(m_pcb.get(), opt);
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

    /* Generic handler */
    template <typename Request, typename State>
    on_request_reply on_request(const Request&, const State&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

protected:
    struct raw_pcb_deleter
    {
        void operator()(raw_pcb* pcb);
    };

    dgram_channel_ptr m_channel; /* shared memory io channel */
    std::unique_ptr<raw_pcb, raw_pcb_deleter> m_pcb; /* lwIP pcb */
    raw_recv_fn m_recv_callback;
    uint8_t m_sock_type;
};

const char* to_string(const raw_socket_state&);

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_RAW_SOCKET_HPP_ */
