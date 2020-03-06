#ifndef _OP_BLOCK_SERVER_HPP_
#define _OP_BLOCK_SERVER_HPP_

#include "core/op_core.h"
#include "json.hpp"
#include "block/device.hpp"
#include "block/block_file.hpp"
#include "block/generator_stack.hpp"

namespace openperf::block::api {

using json = nlohmann::json;

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    std::unique_ptr<device::block_device_stack> blk_device_stack;
    std::unique_ptr<file::block_file_stack> blk_file_stack;
    std::unique_ptr<generator::block_generator_stack> blk_generator_stack;

    void handle_list_devices_request(json& reply);
    void handle_get_device_request(json& request, json& reply);
    void handle_list_block_files_request(json& reply);
    void handle_create_block_file_request(json& request, json& reply);
    void handle_get_block_file_request(json& request, json& reply);
    void handle_delete_block_file_request(json& request, json& reply);
    void handle_list_generators_request(json& reply);
    void handle_create_generator_request(json& request, json& reply);
    void handle_get_generator_request(json& request, json& reply);
    void handle_delete_generator_request(json& request, json& reply);

public:
    server(void* context, core::event_loop& loop);

    int handle_request(const op_event_data* data);
};

} // namespace openperf::block::api

#endif /* _OP_BLOCK_SERVER_HPP_ */
