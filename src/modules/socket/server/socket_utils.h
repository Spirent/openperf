#ifndef _OP_SOCKET_SERVER_SOCKET_UTILS_H_
#define _OP_SOCKET_SERVER_SOCKET_UTILS_H_

#include <optional>
#include <arpa/inet.h>

#include "socket/api.h"

#include "core/op_log.h"
#include "lwip/ip_addr.h"

namespace openperf {
namespace socket {
namespace server {

template <typename Derived, typename StateVariant>
class socket_state_machine
{
    StateVariant m_state;

public:
    struct on_request_reply {
        api::reply_msg msg;
        std::optional<StateVariant> state;
    };

    api::reply_msg handle_request(const api::request_msg& request)
    {
        Derived& child = static_cast<Derived&>(*this);
        auto [msg, next_state] = std::visit(
            [&](const auto& msg, const auto& s) -> on_request_reply {
                return (child.on_request(msg, s));
            },
            request, m_state);
        if (next_state) {
            OP_LOG(OP_LOG_TRACE, "Socket %p: %s --> %s\n",
                    reinterpret_cast<void*>(this), to_string(m_state),
                    to_string(*next_state));
            m_state = *std::move(next_state);
        }
        return (msg);
    }

    const StateVariant& state() const { return (m_state); }

    void state(StateVariant&& state) { m_state = std::move(state); }
    void state(const StateVariant& state) { m_state = state; }
};

std::optional<ip_addr_t> get_address(const sockaddr_storage&);

std::optional<in_port_t> get_port(const sockaddr_storage&);

tl::expected<void, int> copy_in(struct sockaddr_storage& dst,
                                pid_t src_pid, const sockaddr* src_ptr,
                                socklen_t length);

tl::expected<void, int> copy_in(char* dst,
                                pid_t src_pid, const char* src_ptr,
                                socklen_t dstlength, socklen_t srclength);

tl::expected<int, int> copy_in(pid_t src_pid, const int* src_int);

tl::expected<void, int> copy_out(pid_t dst_pid, sockaddr* dst_ptr,
                                 const struct sockaddr_storage& src,
                                 socklen_t length);

tl::expected<void, int> copy_out(pid_t dst_pid, void* dst_ptr, int src);

tl::expected<void, int> copy_out(pid_t dst_pid,
                                 void* dst_ptr, const void* src_ptr,
                                 socklen_t length);

}
}
}

#endif /* _OP_SOCKET_SERVER_SOCKET_UTILS_H_ */
