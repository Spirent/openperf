#ifndef _OP_MEMORY_SERVER_H_
#define _OP_MEMORY_SERVER_H_

#include "json.hpp"
#include "core/op_core.h"

#include "memory/api.hpp"
#include "memory/generator_collection.hpp"

namespace openperf::memory::api {

using json = nlohmann::json;

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<memory::generator_collection> generator_stack;

public:
    server(void* context, core::event_loop& loop);

    int handle_rpc_request(const op_event_data* data);

private:
    // json handle_json_request(const json& request);

    // json list_generators();
    // json create_generator(const json& request);
    // json get_generator(const json& request);
    // json delete_generator(const json& request);
    // json start_generator(const json& request);
    // json stop_generator(const json& request);

    // json bulk_start_generators(const json& request);
    // json bulk_stop_generators(const json& request);

    // json list_results();
    // json get_result(const json& request);
    // json delete_result(const json& request);

    // json get_info();

    // New-style API
    api_reply handle_request(const request::generator::list&);
    api_reply handle_request(const request::generator::get&);
    api_reply handle_request(const request::generator::erase&);
    api_reply handle_request(const request::generator::create&);
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
