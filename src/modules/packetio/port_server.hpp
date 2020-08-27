#ifndef _OP_PACKETIO_PORT_SERVER_HPP_
#define _OP_PACKETIO_PORT_SERVER_HPP_

#include "core/op_core.h"
#include "packetio/generic_driver.hpp"
#include "packetio/port_api.hpp"

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

    reply_msg handle_request(const request_list_ports&);
    reply_msg handle_request(const request_create_port&);
    reply_msg handle_request(const request_get_port&);
    reply_msg handle_request(const request_update_port&);
    reply_msg handle_request(const request_delete_port&);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    driver::generic_driver& m_driver;
};

} // namespace packetio::port::api
} // namespace openperf

#endif /* _OP_PACKETIO_PORT_SERVER_HPP_ */
