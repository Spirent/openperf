#ifndef _OP_PACKETIO_STACK_SERVER_H_
#define _OP_PACKETIO_STACK_SERVER_H_

#include "core/op_core.h"
#include "packetio/generic_stack.h"

namespace openperf {
namespace core {
class event_loop;
}
namespace packetio {
namespace stack {
namespace api {

class server
{
public:
    server(void* context, core::event_loop& loop, stack::generic_stack& stack);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

}
}
}
}
#endif /* _OP_PACKETIO_STACK_SERVER_H_ */
