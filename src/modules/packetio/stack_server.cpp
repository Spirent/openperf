#include <zmq.h>

#include "core/op_core.h"
#include "swagger/v1/model/Stack.h"
#include "packetio/stack_api.hpp"
#include "packetio/stack_server.hpp"

namespace openperf {
namespace packetio {
namespace stack {
namespace api {

const std::string endpoint = "inproc://op_packetio_stack";

using namespace swagger::v1::model;
using json = nlohmann::json;

std::string to_string(request_type type)
{
    const static std::unordered_map<request_type, std::string> request_types = {
        {request_type::LIST_STACKS, "LIST_STACKS"},
        {request_type::GET_STACK, "GET_STACK"},
        {request_type::NONE, "UNKNOWN"} /* Overloaded */
    };

    return (request_types.find(type) == request_types.end()
                ? request_types.at(request_type::NONE)
                : request_types.at(type));
}

std::string to_string(reply_code code)
{
    const static std::unordered_map<reply_code, std::string> reply_codes = {
        {reply_code::OK, "OK"},
        {reply_code::NO_STACK, "NO_STACK_ID"},
        {reply_code::ERROR, "ERROR"},
        {reply_code::NONE, "UNKNOWN"}, /* Overloaded */
    };

    return (reply_codes.find(code) == reply_codes.end()
                ? reply_codes.at(reply_code::NONE)
                : reply_codes.at(code));
}

template <typename T>
static std::optional<T> get_optional_key(json& j, const char* key)
{
    return (j.find(key) != j.end() && !j[key].is_null()
                ? std::make_optional(j[key].get<T>())
                : std::nullopt);
}

static void handle_list_stacks_request(generic_stack& stack,
                                       json& request __attribute__((unused)),
                                       json& reply)
{
    json jstacks = json::array();
    jstacks.emplace_back(make_swagger_stack(stack)->toJson());
    reply["code"] = reply_code::OK;
    reply["data"] = jstacks.dump();
}

static void
handle_get_stack_request(generic_stack& stack, json& request, json& reply)
{
    auto id = request["id"].get<std::string>();
    if (id == stack.id()) {
        reply["code"] = reply_code::OK;
        reply["data"] = make_swagger_stack(stack)->toJson().dump();
    } else {
        reply["code"] = reply_code::NO_STACK;
    }
}

static int handle_rpc_request(const op_event_data* data, void* arg)
{
    generic_stack& stack = *(reinterpret_cast<generic_stack*>(arg));
    int recv_or_err = 0;
    int send_or_err = 0;
    zmq_msg_t request_msg;
    if (zmq_msg_init(&request_msg) == -1) {
        throw std::runtime_error(zmq_strerror(errno));
    }
    while (
        (recv_or_err = zmq_msg_recv(&request_msg, data->socket, ZMQ_DONTWAIT))
        > 0) {
        std::vector<uint8_t> request_buffer(
            static_cast<uint8_t*>(zmq_msg_data(&request_msg)),
            static_cast<uint8_t*>(zmq_msg_data(&request_msg))
                + zmq_msg_size(&request_msg));

        json request = json::from_cbor(request_buffer);
        request_type type = request["type"].get<request_type>();
        json reply;

        switch (type) {
        case request_type::GET_STACK:
            OP_LOG(OP_LOG_TRACE,
                   "Received %s request for port %s\n",
                   to_string(type).c_str(),
                   request["id"].get<std::string>().c_str());
            break;
        default:
            OP_LOG(
                OP_LOG_TRACE, "Received %s request\n", to_string(type).c_str());
        }

        switch (type) {
        case request_type::LIST_STACKS:
            handle_list_stacks_request(stack, request, reply);
            break;
        case request_type::GET_STACK:
            handle_get_stack_request(stack, request, reply);
            break;
        default:
            reply["code"] = reply_code::ERROR;
            reply["error"] = json_error(
                ENOSYS, "Request type not implemented in packtio port server");
        }

        std::vector<uint8_t> reply_buffer = json::to_cbor(reply);
        if ((send_or_err = zmq_send(
                 data->socket, reply_buffer.data(), reply_buffer.size(), 0))
            != static_cast<int>(reply_buffer.size())) {
            OP_LOG(OP_LOG_ERROR,
                   "Request reply failed: %s\n",
                   zmq_strerror(errno));
        } else {
            OP_LOG(OP_LOG_TRACE,
                   "Sent %s reply to %s request\n",
                   to_string(reply["code"].get<reply_code>()).c_str(),
                   to_string(type).c_str());
        }
    }

    zmq_msg_close(&request_msg);

    return (((recv_or_err < 0 || send_or_err < 0) && errno == ETERM) ? -1 : 0);
}

server::server(void* context,
               openperf::core::event_loop& loop,
               generic_stack& stack)
    : m_socket(op_socket_get_server(context, ZMQ_REP, endpoint.c_str()))
{
    struct op_event_callbacks callbacks = {.on_read = handle_rpc_request};
    loop.add(m_socket.get(), &callbacks, &stack);
}

} // namespace api
} // namespace stack
} // namespace packetio
} // namespace openperf
