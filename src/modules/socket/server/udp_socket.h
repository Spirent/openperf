#ifndef _ICP_SOCKET_SERVER_UDP_SOCKET_H_
#define _ICP_SOCKET_SERVER_UDP_SOCKET_H_

#include <cerrno>
#include <optional>
#include <utility>
#include <variant>

#include "tl/expected.hpp"

#include "socket/server/socket_utils.h"

struct udp_pcb;
struct pbuf;

namespace icp {
namespace socket {
namespace server {

class io_channel;

struct udp_init {};
struct udp_bound {};
struct udp_connected {};
struct udp_bound_and_connected {};
struct udp_closed {};

typedef std::variant<udp_init,
                     udp_bound,
                     udp_connected,
                     udp_bound_and_connected,
                     udp_closed> udp_socket_state;

class udp_socket : public socket_state_machine<udp_socket, udp_socket_state> {
    udp_pcb *m_pcb;

public:
    udp_socket(io_channel*);
    ~udp_socket();

    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&&) = delete;

    /* bind handlers */
    on_request_reply on_request(const api::request_bind&, const udp_init&);
    on_request_reply on_request(const api::request_bind&, const udp_connected&);

    /* connect handlers */
    on_request_reply on_request(const api::request_connect&, const udp_init&);
    on_request_reply on_request(const api::request_connect&, const udp_bound&);
    on_request_reply on_request(const api::request_connect&, const udp_bound_and_connected&);
    on_request_reply on_request(const api::request_connect&, const udp_connected&);

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&, const udp_connected&);
    on_request_reply on_request(const api::request_getpeername&, const udp_bound_and_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&, const udp_bound&);
    on_request_reply on_request(const api::request_getsockname&, const udp_bound_and_connected&);

    /* getsockopt handlers */
    static tl::expected<socklen_t, int> do_udp_getsockopt(udp_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& opt, const State&)
    {
        auto result = do_udp_getsockopt(m_pcb, opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    static tl::expected<void, int> do_udp_setsockopt(udp_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& opt, const State&)
    {
        auto result = do_udp_setsockopt(m_pcb, opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* Generic handler */
    template <typename Request, typename State>
    on_request_reply on_request(const Request&, const State&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    void handle_transmit(pid_t pid, io_channel* channel);
};

const char * to_string(const udp_socket_state&);

/**
 * Generic receive function used by lwIP upon packet reception.  This
 * is added to the pcb when the udp_socket object is constructed.
 */
void udp_receive(void*, udp_pcb*, pbuf*, const ip_addr_t*, in_port_t);

}
}
}

#endif /* _ICP_SOCKET_SERVER_UDP_SOCKET_H_ */
