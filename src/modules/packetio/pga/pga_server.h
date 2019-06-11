#ifndef _ICP_PACKETIO_PGA_SERVER_H_
#define _ICP_PACKETIO_PGA_SERVER_H_

#include <memory>

#include "packetio/generic_driver.h"
#include "packetio/generic_stack.h"

#include "core/icp_core.h"

namespace icp {

namespace core { class event_loop; }

namespace packetio::pga::api {

class server
{
    std::unique_ptr<void, icp_socket_deleter> m_socket;
    driver::generic_driver& m_driver;
    stack::generic_stack& m_stack;

public:
    server(void *context, core::event_loop& loop,
           driver::generic_driver& driver,
           stack::generic_stack& stack);

    driver::generic_driver& driver() const;
    stack::generic_stack&   stack() const;
};

}
}

#endif /* _ICP_PACKETIO_PGA_SERVER_H_ */
