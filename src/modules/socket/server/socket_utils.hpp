#ifndef _OP_SOCKET_SERVER_SOCKET_UTILS_HPP_
#define _OP_SOCKET_SERVER_SOCKET_UTILS_HPP_

#include <optional>
#include <arpa/inet.h>

#include "core/op_log.h"
#include "lwip/ip_addr.h"

#include "socket/api.hpp"
#include "socket/server/generic_socket.hpp"

namespace openperf::socket::server {

template <typename Derived, typename StateVariant> class socket_state_machine
{
    StateVariant m_state;

public:
    struct on_request_reply
    {
        api::reply_msg msg;
        std::optional<StateVariant> state;
    };

    api::reply_msg handle_request(const api::request_msg& request)
    {
        auto& child = static_cast<Derived&>(*this);
        auto [msg, next_state] = std::visit(
            [&](const auto& msg, const auto& s) -> on_request_reply {
                return (child.on_request(msg, s));
            },
            request,
            m_state);
        if (next_state) {
            OP_LOG(OP_LOG_TRACE,
                   "Socket %p: %s --> %s\n",
                   reinterpret_cast<void*>(this),
                   to_string(m_state),
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
                                pid_t src_pid,
                                const sockaddr* src_ptr,
                                socklen_t length);

tl::expected<void, int> copy_in(char* dst,
                                pid_t src_pid,
                                const char* src_ptr,
                                socklen_t dstlength,
                                socklen_t srclength);

template <typename T>
tl::expected<void, int> copy_in(T& dst, pid_t src_pid, const void* src_ptr)
{
    return (copy_in(reinterpret_cast<char*>(std::addressof(dst)),
                    src_pid,
                    reinterpret_cast<const char*>(src_ptr),
                    sizeof(dst),
                    sizeof(dst)));
}

tl::expected<int, int> copy_in(pid_t src_pid, const int* src_int);

tl::expected<void, int> copy_out(pid_t dst_pid,
                                 sockaddr* dst_ptr,
                                 const struct sockaddr_storage& src,
                                 socklen_t length);

tl::expected<void, int> copy_out(pid_t dst_pid, void* dst_ptr, int src);

tl::expected<void, int>
copy_out(pid_t dst_pid, void* dst_ptr, const void* src_ptr, socklen_t length);

template <typename T>
tl::expected<void, int> copy_out(pid_t dst_pid, void* dst_ptr, const T& src)
{
    return (copy_out(dst_pid, dst_ptr, std::addressof(src), sizeof(src)));
}

tl::expected<generic_socket, int>
make_socket_common(allocator& allocator, int domain, int type, int protocol);

} // namespace openperf::socket::server

#endif /* _OP_SOCKET_SERVER_SOCKET_UTILS_HPP_ */
