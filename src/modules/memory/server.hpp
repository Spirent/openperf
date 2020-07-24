#ifndef _OP_MEMORY_SERVER_H_
#define _OP_MEMORY_SERVER_H_

#include "api.hpp"
#include "generator_collection.hpp"

#include "framework/core/op_core.h"

namespace openperf::memory::api {

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<memory::generator_collection> m_generator_stack;

public:
    server(void* context, core::event_loop& loop);

    int handle_rpc_request(const op_event_data* data);

private:
    api_reply handle_request(const request::generator::list&);
    api_reply handle_request(const request::generator::get&);
    api_reply handle_request(const request::generator::erase&);
    api_reply handle_request(const request::generator::create&);

    api_reply handle_request(const request::generator::bulk::create&);
    api_reply handle_request(const request::generator::bulk::erase&);

    api_reply handle_request(const request::generator::stop&);
    api_reply handle_request(const request::generator::start&);

    api_reply handle_request(const request::generator::bulk::start&);
    api_reply handle_request(const request::generator::bulk::stop&);

    api_reply handle_request(const request::statistic::list&);
    api_reply handle_request(const request::statistic::get&);
    api_reply handle_request(const request::statistic::erase&);

    api_reply handle_request(const request::info&);
};

} // namespace openperf::memory::api

#endif // _OP_MEMORY_SERVER_H_
