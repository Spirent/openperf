#ifndef _OP_CPU_SERVER_HPP_
#define _OP_CPU_SERVER_HPP_

#include <map>
#include <memory>

#include "framework/core/op_core.h"

#include "cpu/api.hpp"
#include "cpu/generator/coordinator.hpp"

namespace openperf::cpu::api {

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

    reply_msg handle_request(const request_cpu_generator_list&);
    reply_msg handle_request(const request_cpu_generator&);
    reply_msg handle_request(const request_cpu_generator_add&);
    reply_msg handle_request(const request_cpu_generator_del&);
    reply_msg handle_request(const request_cpu_generator_bulk_add&);
    reply_msg handle_request(const request_cpu_generator_bulk_del&);
    reply_msg handle_request(const request_cpu_generator_start&);
    reply_msg handle_request(const request_cpu_generator_stop&);
    reply_msg handle_request(const request_cpu_generator_bulk_start&);
    reply_msg handle_request(const request_cpu_generator_bulk_stop&);
    reply_msg handle_request(const request_cpu_generator_result_list&);
    reply_msg handle_request(const request_cpu_generator_result&);
    reply_msg handle_request(const request_cpu_generator_result_del&);
};

} // namespace openperf::cpu::api

#endif // _OP_CPU_SERVER_HPP_
