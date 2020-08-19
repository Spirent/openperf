#ifndef _OP_TVLP_SERVER_H_
#define _OP_TVLP_SERVER_H_

#include "api.hpp"
#include "controller_stack.hpp"
#include "framework/core/op_core.h"

namespace openperf::tvlp::api {

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<internal::controller_stack> m_controller_stack;

public:
    server(void* context, core::event_loop& loop);

    int handle_rpc_request(const op_event_data* data);

private:
    api_reply handle_request(const request::tvlp::list&);
    api_reply handle_request(const request::tvlp::get&);
    api_reply handle_request(const request::tvlp::erase&);
    api_reply handle_request(const request::tvlp::create&);
    api_reply handle_request(const request::tvlp::start&);
    api_reply handle_request(const request::tvlp::stop&);
    api_reply handle_request(const request::tvlp::result::list&);
    api_reply handle_request(const request::tvlp::result::get&);
    api_reply handle_request(const request::tvlp::result::erase&);
};

} // namespace openperf::tvlp::api

#endif // _OP_MEMORY_SERVER_H_
