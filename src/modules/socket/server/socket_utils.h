#ifndef _ICP_SOCKET_SERVER_SOCKET_UTILS_H_
#define _ICP_SOCKET_SERVER_SOCKET_UTILS_H_

#include "socket/api.h"

namespace icp {
namespace socket {
namespace server {

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

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
            [&](auto& s) -> on_request_reply {
                return (child.on_request(s, request));
            },
            m_state);
        if (next_state) {
            m_state = *std::move(next_state);
        }
        return (msg);
    }
};

}
}
}

#endif /* _ICP_SOCKET_SERVER_SOCKET_UTILS_H_ */
