#ifndef _OP_BLOCK_SERVER_HPP_
#define _OP_BLOCK_SERVER_HPP_

#include "framework/core/op_core.h"

#include "api.hpp"
#include "device_stack.hpp"
#include "file_stack.hpp"
#include "generator_stack.hpp"

namespace openperf::block::api {

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<device::device_stack> m_device_stack;
    std::unique_ptr<file::file_stack> m_file_stack;
    std::unique_ptr<generator::generator_stack> m_generator_stack;

public:
    server(void* context, core::event_loop& loop);

    reply_msg handle_request(const request_block_device&);
    reply_msg handle_request(const request_block_device_list&);
    reply_msg handle_request(const request_block_device_init&);
    reply_msg handle_request(const request_block_file_list&);
    reply_msg handle_request(const request_block_file&);
    reply_msg handle_request(const request_block_file_add&);
    reply_msg handle_request(const request_block_file_del&);
    reply_msg handle_request(const request_block_file_bulk_add&);
    reply_msg handle_request(const request_block_file_bulk_del&);
    reply_msg handle_request(const request_block_generator_list&);
    reply_msg handle_request(const request_block_generator&);
    reply_msg handle_request(const request_block_generator_add&);
    reply_msg handle_request(const request_block_generator_del&);
    reply_msg handle_request(const request_block_generator_bulk_add&);
    reply_msg handle_request(const request_block_generator_bulk_del&);
    reply_msg handle_request(const request_block_generator_start&);
    reply_msg handle_request(const request_block_generator_stop&);
    reply_msg handle_request(const request_block_generator_bulk_start&);
    reply_msg handle_request(const request_block_generator_bulk_stop&);
    reply_msg handle_request(const request_block_generator_result_list&);
    reply_msg handle_request(const request_block_generator_result&);
    reply_msg handle_request(const request_block_generator_result_del&);
};

} // namespace openperf::block::api

#endif /* _OP_BLOCK_SERVER_HPP_ */
