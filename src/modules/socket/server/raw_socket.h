#ifndef _ICP_SOCKET_SERVER_RAW_SOCKET_H_
#define _ICP_SOCKET_SERVER_RAW_SOCKET_H_

#include <cerrno>
#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include <linux/icmp.h>

#include "tl/expected.hpp"

#include "socket/server/allocator.h"
#include "socket/server/dgram_channel.h"
#include "socket/server/generic_socket.h"
#include "socket/server/socket_utils.h"

struct raw_pcb;
struct pbuf;

namespace icp {
namespace socket {
namespace server {

struct raw_init {};
struct raw_bound {};
struct raw_connected {};
struct raw_closed {};

typedef std::variant<raw_init,
                     raw_bound,
                     raw_connected,
                     raw_closed> raw_socket_state;

class raw_socket : public socket_state_machine<raw_socket, raw_socket_state> {
    struct raw_pcb_deleter {
        void operator()(raw_pcb *pcb);
    };

    dgram_channel_ptr m_channel;                      /* shared memory io channel */
    std::unique_ptr<raw_pcb, raw_pcb_deleter> m_pcb;  /* lwIP pcb */

public:
    raw_socket(icp::socket::server::allocator& allocator, int flags, int protocol);
    ~raw_socket() = default;

    raw_socket(const raw_socket&) = delete;
    raw_socket& operator=(const raw_socket&&) = delete;

    raw_socket& operator=(raw_socket&& other) noexcept;
    raw_socket(raw_socket&& other) noexcept;

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
    on_request_reply on_request(const api::request_bind&, const raw_init&);

    /* connect handlers */
    on_request_reply on_request(const api::request_connect&, const raw_init&);
    on_request_reply on_request(const api::request_connect&, const raw_bound&);
    on_request_reply on_request(const api::request_connect&, const raw_connected&);

    /* getpeername handlers */
    on_request_reply on_request(const api::request_getpeername&, const raw_connected&);

    template <typename State>
    on_request_reply on_request(const api::request_getpeername&, const State&)
    {
        return {tl::make_unexpected(ENOTCONN), std::nullopt};
    }

    /* getsockname handlers */
    on_request_reply on_request(const api::request_getsockname&, const raw_bound&);
    on_request_reply on_request(const api::request_getsockname&, const raw_connected&);

    /* getsockopt handlers */
    tl::expected<socklen_t, int> do_getsockopt(const raw_pcb*, const api::request_getsockopt&);

    template <typename State>
    on_request_reply on_request(const api::request_getsockopt& opt, const State&)
    {
        auto result = do_getsockopt(m_pcb.get(), opt);
        if (!result) return {tl::make_unexpected(result.error()), std::nullopt};
        return {api::reply_socklen{*result}, std::nullopt};
    }

    /* setsockopt handlers */
    tl::expected<void, int> do_setsockopt(raw_pcb*, const api::request_setsockopt&);

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

    uint32_t icmp_filter();

    bool option(int);
    
private:
    struct icmp_filter m_icmp_filter;
    uint64_t m_options;

};

const char * to_string(const raw_socket_state&);

/**
 * Generic receive function used by lwIP upon packet reception.  This
 * is added to the pcb when the raw_socket object is constructed.
 */
uint8_t raw_receive(void*, raw_pcb*, pbuf*, const ip_addr_t*);

}
}
}

#endif /* _ICP_SOCKET_SERVER_RAW_SOCKET_H_ */
