#ifndef _OP_CPU_SERVER_HPP_
#define _OP_CPU_SERVER_HPP_

#include "json.hpp"
#include "core/op_core.h"

#include "api.hpp"
#include "generator_stack.hpp"

namespace openperf::cpu::api {

using generator_stack = cpu::generator::generator_stack;
using json = nlohmann::json;

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<generator_stack> m_generator_stack;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_cpu_generator_list&);
    reply_msg handle_request(const request_cpu_generator&);
    reply_msg handle_request(const request_cpu_generator_add&);
    reply_msg handle_request(const request_cpu_generator_del&);
    reply_msg handle_request(const request_cpu_generator_start&);
    reply_msg handle_request(const request_cpu_generator_stop&);
    reply_msg handle_request(const request_cpu_generator_bulk_start&);
    reply_msg handle_request(const request_cpu_generator_bulk_stop&);
    reply_msg handle_request(const request_cpu_generator_result_list&);
    reply_msg handle_request(const request_cpu_generator_result&);
    reply_msg handle_request(const request_cpu_generator_result_del&);
    reply_msg handle_request(const request_cpu_info&);
};

} // namespace openperf::cpu::api

#endif /* _OP_CPU_SERVER_HPP_ */
