#ifndef _OP_BLOCK_SERVER_HPP_
#define _OP_BLOCK_SERVER_HPP_

#include "core/op_core.h"
#include "json.hpp"
#include "block/block_file.hpp"

namespace openperf::block::api {

class server
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
    file::block_file_stack& blk_file_stack;

    void handle_list_devices_request(nlohmann::json& reply);
    void handle_get_device_request(nlohmann::json& request, nlohmann::json& reply);
    void handle_list_block_files_request(nlohmann::json& reply);
    void handle_create_block_file_request(nlohmann::json& request, nlohmann::json& reply);
    void handle_get_block_file_request(nlohmann::json& request, nlohmann::json& reply);
    void handle_delete_block_file_request(nlohmann::json& request, nlohmann::json& reply);

public:
    server(void* context, core::event_loop& loop, block::file::block_file_stack& blk_file_stack);
    
    int handle_request(const op_event_data* data);
};

} // namespace openperf::block::api

#endif /* _OP_BLOCK_SERVER_HPP_ */
