#ifndef _ICP_SOCKET_SERVER_UDP_SOCKET_H_
#define _ICP_SOCKET_SERVER_UDP_SOCKET_H_

#include <cerrno>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "tl/expected.hpp"

#include "socket/server/allocator.h"
#include "socket/server/dgram_channel.h"
#include "socket/server/generic_socket.h"
#include "socket/server/socket_utils.h"

struct udp_pcb;
struct pbuf;

namespace icp {
namespace socket {
namespace server {

struct udp_init {};
struct udp_bound {};
struct udp_connected {};
struct udp_closed {};

typedef std::variant<udp_init,
                     udp_bound,
                     udp_connected,
                     udp_closed> udp_socket_state;

class udp_socket : public socket_state_machine<udp_socket, udp_socket_state> {
    struct udp_pcb_deleter {
        void operator()(udp_pcb *pcb);
    };

    dgram_channel_ptr m_channel;                      /* shared memory io channel */
    std::unique_ptr<udp_pcb, udp_pcb_deleter> m_pcb;  /* lwIP pcb */

public:
    udp_socket(icp::socket::server::allocator& allocator, int flags);
    ~udp_socket() = default;

    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&&) = delete;

    udp_socket& operator=(udp_socket&& other) noexcept;
    udp_socket(udp_socket&& other) noexcept;

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
    on_request_reply on_request(const api::request_bind&, const udp_init&);

    /* connect handlers */
    on_request_reply on_request(const api::request_connect&, const udp_init&);
    on_request_reply on_request(const api::request_connect&, const udp_bound&);
    on_request_reply on_request(const api::request_connect&, const udp_connected&);

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&, const udp_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&, const udp_bound&);
    on_request_reply on_request(const api::request_getsockname&, const udp_connected&);

    /* getsockopt handlers */
    static tl::expected<socklen_t, int> do_getsockopt(const udp_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& opt, const State&)
    {
        auto result = do_getsockopt(m_pcb.get(), opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    tl::expected<void, int> do_setsockopt(udp_pcb*, const api::request_setsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_setsockopt& opt, const State&)
    {
        auto result = do_setsockopt(m_pcb.get(), opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_success(), std::nullopt};
    }

    /* Generic handler */
    template <typename Request, typename State>
    on_request_reply on_request(const Request&, const State&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    bool option(int);

private:
    uint64_t m_options;

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
