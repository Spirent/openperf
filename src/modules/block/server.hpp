#ifndef _OP_BLOCK_SERVER_HPP_
#define _OP_BLOCK_SERVER_HPP_

#include "json.hpp"
#include "core/op_core.h"

#include "api.hpp"
#include "device_stack.hpp"
#include "file_stack.hpp"
#include "generator_stack.hpp"

namespace openperf::block::api {

using json = nlohmann::json;
using device_stack = block::device::device_stack;
using file_stack = block::file::file_stack;
class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    device_stack* blk_device_stack;
    file_stack* blk_file_stack;
    std::unique_ptr<generator::generator_stack> blk_generator_stack;

    void handle_list_generators_request(json& reply);
    void handle_create_generator_request(json& request, json& reply);
    void handle_get_generator_request(json& request, json& reply);
    void handle_delete_generator_request(json& request, json& reply);
    void handle_start_generator_request(json& request, json& reply);
    void handle_stop_generator_request(json& request, json& reply);
    void handle_bulk_start_generators_request(json& request, json& reply);
    void handle_bulk_stop_generators_request(json& request, json& reply);
    void handle_list_generator_results_request(json& reply);
    void handle_get_generator_result_request(json& request, json& reply);
    void handle_delete_generator_result_request(json& request, json& reply);
public:
    server(void* context, core::event_loop& loop);

    int handle_request(const op_event_data* data);
    reply_msg handle_request(const request_block_device&);
    reply_msg handle_request(const request_block_device_list&);
    reply_msg handle_request(const request_block_file_list&);
    reply_msg handle_request(const request_block_file&);
    reply_msg handle_request(const request_block_file_add&);
    reply_msg handle_request(const request_block_file_del&);

};

} // namespace openperf::block::api

#endif /* _OP_BLOCK_SERVER_HPP_ */
