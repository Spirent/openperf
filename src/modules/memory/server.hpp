#ifndef _OP_MEMORY_SERVER_H_
#define _OP_MEMORY_SERVER_H_

#include <map>
#include <memory>

#include "framework/core/op_core.h"

#include "memory/api.hpp"
#include "memory/generator/coordinator.hpp"

namespace openperf::memory::api {

class server
{
    void* m_context;
    core::event_loop& m_loop;
    std::unique_ptr<void, op_socket_deleter> m_socket;

    using coordinator_key = std::string;
    using coordinator_value_type = generator::coordinator;
    using coordinator_value_ptr = std::unique_ptr<coordinator_value_type>;
    using coordinator_map = std::map<coordinator_key, coordinator_value_ptr>;

    using result_key = core::uuid;
    using result_value_type = generator::result;
    using result_value_ptr = std::shared_ptr<result_value_type>;
    using result_map = std::map<result_key, result_value_ptr>;

    using coordinator_result_map = std::multimap<coordinator_key, result_key>;

    /* coordinator key -> coordinator */
    coordinator_map m_coordinators;
    /* result key -> result */
    result_map m_results;
    /* coordinator key -> result keys */
    coordinator_result_map m_coordinator_results;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request::generator::list&);
    reply_msg handle_request(const request::generator::get&);
    reply_msg handle_request(const request::generator::erase&);
    reply_msg handle_request(const request::generator::create&);

    reply_msg handle_request(const request::generator::bulk::create&);
    reply_msg handle_request(const request::generator::bulk::erase&);

    reply_msg handle_request(const request::generator::stop&);
    reply_msg handle_request(const request::generator::start&);

    reply_msg handle_request(const request::generator::bulk::start&);
    reply_msg handle_request(const request::generator::bulk::stop&);

    reply_msg handle_request(const request::result::list&);
    reply_msg handle_request(const request::result::get&);
    reply_msg handle_request(const request::result::erase&);
};

} // namespace openperf::memory::api

#endif // _OP_MEMORY_SERVER_H_
