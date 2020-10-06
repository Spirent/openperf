#ifndef _OP_PACKET_STACK_SERVER_HPP_
#define _OP_PACKET_STACK_SERVER_HPP_

#include "core/op_core.h"
#include "packet/stack/api.hpp"
#include "packet/stack/generic_stack.hpp"

namespace openperf::packet::stack::api {

class server
{
    std::unique_ptr<void, op_socket_deleter> m_socket;
    core::event_loop& m_loop;
    generic_stack& m_stack;

public:
    server(void* context, core::event_loop& loop, generic_stack& stack);

    reply_msg handle_request(const request_list_interfaces&);
    reply_msg handle_request(const request_create_interface&);
    reply_msg handle_request(const request_get_interface&);
    reply_msg handle_request(const request_delete_interface&);

    reply_msg handle_request(const request_bulk_create_interfaces&);
    reply_msg handle_request(const request_bulk_delete_interfaces&);

    reply_msg handle_request(const request_list_stacks&);
    reply_msg handle_request(const request_get_stack&);
};

} // namespace openperf::packet::stack::api

#endif /* _OP_PACKET_STACK_SERVER_HPP_ */
