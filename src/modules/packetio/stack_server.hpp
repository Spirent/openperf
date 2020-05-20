#ifndef _OP_PACKETIO_STACK_SERVER_HPP_
#define _OP_PACKETIO_STACK_SERVER_HPP_

#include "core/op_core.h"
#include "packetio/generic_stack.hpp"

namespace openperf {
namespace core {
class event_loop;
}
namespace packetio::stack::api {

class server
{
public:
    server(void* context, core::event_loop& loop, stack::generic_stack& stack);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

} // namespace packetio::stack::api
} // namespace openperf
#endif /* _OP_PACKETIO_STACK_SERVER_HPP_ */
