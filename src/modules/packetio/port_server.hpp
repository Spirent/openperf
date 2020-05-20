#ifndef _OP_PACKETIO_PORT_SERVER_HPP_
#define _OP_PACKETIO_PORT_SERVER_HPP_

#include "core/op_core.h"
#include "packetio/generic_driver.hpp"

namespace openperf {

namespace core {
class event_loop;
}

namespace packetio::port::api {

class server
{
public:
    server(void* context,
           core::event_loop& loop,
           driver::generic_driver& driver);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

} // namespace packetio::port::api
} // namespace openperf

#endif /* _OP_PACKETIO_PORT_SERVER_HPP_ */
