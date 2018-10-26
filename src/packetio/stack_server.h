#ifndef _ICP_PACKETIO_STACK_SERVER_H_
#define _ICP_PACKETIO_STACK_SERVER_H_

#include "core/icp_core.h"
#include "packetio/generic_stack.h"

namespace icp {
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
    std::unique_ptr<void, icp_socket_deleter> m_socket;
};

}
}
}
}
#endif /* _ICP_PACKETIO_STACK_SERVER_H_ */
