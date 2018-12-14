#ifndef _ICP_SOCKET_SERVER_TCP_SOCKET_H_
#define _ICP_SOCKET_SERVER_TCP_SOCKET_H_

#include <cerrno>
#include "tl/expected.hpp"

#include "socket/server/socket_utils.h"

struct tcp_pcb;

namespace icp {
namespace socket {
namespace server {

struct tcp_init {};

typedef std::variant<tcp_init> tcp_socket_state;

class tcp_socket : public socket_state_machine<tcp_socket, tcp_socket_state>
{
    tcp_pcb* m_pcb;

public:
    tcp_socket();
    ~tcp_socket();

    tcp_socket(const tcp_socket&) = delete;
    tcp_socket& operator=(const tcp_socket&&) = delete;

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

#endif /* _ICP_SOCKET_SERVER_TCP_SOCKET_H_ */
