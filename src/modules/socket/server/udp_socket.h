#ifndef _ICP_SOCKET_SERVER_UDP_SOCKET_H_
#define _ICP_SOCKET_SERVER_UDP_SOCKET_H_

#include <cerrno>
#include <optional>
#include <utility>
#include <variant>

#include "tl/expected.hpp"

#include "socket/server/socket_utils.h"

struct udp_pcb;

namespace icp {
namespace socket {
namespace server {

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
    udp_socket();
    ~udp_socket();

    udp_socket(const udp_socket&) = delete;
    udp_socket& operator=(const udp_socket&&) = delete;

    on_request_reply on_request(udp_init&, const api::request_bind&);
    on_request_reply on_request(udp_init&, const api::request_connect&);

#if 0
    std::optional<udp_socket_state> on_request(udp_bound&, const api::request_connect&);
    std::optional<udp_socket_state> on_request(udp_connected&, const api::request_connect&);
    std::optional<udp_socket_state> on_request(udp_bound_and_connected&, const api::request_connect&);
#endif

    /* Generic handler */
    template <typename State, typename Request>
    on_request_reply on_request(State&, const Request&)
    {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_UDP_SOCKET_H_ */
