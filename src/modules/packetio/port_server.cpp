#include <cerrno>
#include <optional>
#include <unordered_map>

#include "zmq.h"

#include "core/icp_core.h"
#include "swagger/v1/model/Port.h"
#include "packetio/json_transmogrify.h"
#include "packetio/port_api.h"
#include "packetio/port_server.h"

namespace icp {
namespace packetio {
namespace port {
namespace api {

const std::string endpoint = "inproc://icp_packetio_port";

using namespace swagger::v1::model;
using json = nlohmann::json;
using generic_driver = icp::packetio::driver::generic_driver;

std::string to_string(request_type type)
{
    const static std::unordered_map<request_type, std::string> request_types = {
        { request_type::LIST_PORTS,  "LIST_PORTS"  },
        { request_type::CREATE_PORT, "CREATE_PORT" },
        { request_type::GET_PORT,    "GET_PORT"    },
        { request_type::UPDATE_PORT, "UPDATE_PORT" },
        { request_type::DELETE_PORT, "DELETE_PORT" },
        { request_type::NONE,        "UNKNOWN"     }  /* Overloaded */
    };

    return (request_types.find(type) == request_types.end()
            ? request_types.at(request_type::NONE)
            : request_types.at(type));
}

std::string to_string(reply_code code)
{
    const static std::unordered_map<reply_code, std::string> reply_codes = {
        { reply_code::OK,        "OK"         },
        { reply_code::NO_PORT,   "NO_PORT_ID" },
        { reply_code::BAD_INPUT, "BAD_INPUT"  },
        { reply_code::ERROR,     "ERROR"      },
        { reply_code::NONE,      "UNKNOWN"    },  /* Overloaded */
    };

    return (reply_codes.find(code) == reply_codes.end()
            ? reply_codes.at(reply_code::NONE)
            : reply_codes.at(code));
}

template<typename T>
static std::optional<T> get_optional_key(json& j, const char * key)
{
    return (j.find(key) != j.end() && !j[key].is_null()
            ? std::make_optional(j[key].get<T>())
            : std::nullopt);
}

static void _handle_list_ports_request(generic_driver& driver, json& request, json& reply)
{
    auto kind = get_optional_key<std::string>(request, "kind");
    json jports = json::array();

    for (int id : driver.port_ids()) {
        auto port = driver.port(id);
        if (!kind || *kind == port->kind()) {
            jports.emplace_back(make_swagger_port(*port)->toJson());
        }
    }

    reply["code"] = reply_code::OK;
    reply["data"] = jports.dump();
}

static void _handle_create_port_request(generic_driver& driver, json& request, json& reply)
{
    try {
        auto port_model = json::parse(request["data"].get<std::string>()).get<Port>();
        auto result = driver.create_port(make_config_data(port_model));
        if (!result) {
            throw std::runtime_error(result.error().c_str());
        }
        reply["code"] = reply_code::OK;
        reply["data"] = make_swagger_port(*driver.port(result.value()))->toJson().dump();
    } catch (const json::exception& e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(e.id, e.what());
    } catch (const std::runtime_error& e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(EINVAL, e.what());
    }
}

static void _handle_get_port_request(generic_driver& driver, json& request, json& reply)
{
    int id = request["id"].get<int>();
    auto port = driver.port(id);
    if (port) {
        reply["code"] = reply_code::OK;
        reply["data"] = make_swagger_port(*port)->toJson().dump();
    } else {
        reply["code"] = reply_code::NO_PORT;
    }
}

static void _handle_update_port_request(generic_driver& driver, json& request, json& reply)
{
    int id = request["id"].get<int>();
    auto port = driver.port(id);
    if (!port) {
        reply["code"] = reply_code::NO_PORT;
        return;
    }

    try {
        auto port_model = json::parse(request["data"].get<std::string>()).get<Port>();
        auto result = port->config(make_config_data(port_model));
        if (!result) {
            throw std::runtime_error(result.error().c_str());
        }
        reply["code"] = reply_code::OK;
        reply["data"] = make_swagger_port(*port)->toJson().dump();
    } catch (const json::exception& e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(e.id, e.what());
    } catch (const std::runtime_error& e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(EINVAL, e.what());
    }
}

static void _handle_delete_port_request(generic_driver& driver, json& request, json& reply)
{
    auto result = driver.delete_port(request["id"].get<int>());
    if (result) {
        reply["code"] = reply_code::OK;
    } else {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(EINVAL, result.error().c_str());
    }
}

static int _handle_rpc_request(const icp_event_data *data, void *arg)
{
    generic_driver& driver = *(reinterpret_cast<generic_driver *>(arg));
    int recv_or_err = 0;
    int send_or_err = 0;
    zmq_msg_t request_msg;
    if (zmq_msg_init(&request_msg) == -1) {
        throw std::runtime_error(zmq_strerror(errno));
    }
    while ((recv_or_err = zmq_msg_recv(&request_msg, data->socket, ZMQ_DONTWAIT)) > 0) {
        std::vector<uint8_t> request_buffer(static_cast<uint8_t *>(zmq_msg_data(&request_msg)),
                                            static_cast<uint8_t *>(zmq_msg_data(&request_msg)) + zmq_msg_size(&request_msg));

        json request = json::from_cbor(request_buffer);
        request_type type = request["type"].get<request_type>();
        json reply;

        switch (type) {
        case request_type::GET_PORT:
        case request_type::UPDATE_PORT:
        case request_type::DELETE_PORT:
            ICP_LOG(ICP_LOG_TRACE, "Received %s request for port %d\n",
                    to_string(type).c_str(),
                    request["id"].get<int>());
            break;
        default:
            ICP_LOG(ICP_LOG_TRACE, "Received %s request\n", to_string(type).c_str());
        }

        switch (type) {
        case request_type::LIST_PORTS:
            _handle_list_ports_request(driver, request, reply);
            break;
        case request_type::CREATE_PORT:
            _handle_create_port_request(driver, request, reply);
            break;
        case request_type::GET_PORT:
            _handle_get_port_request(driver, request, reply);
            break;
        case request_type::UPDATE_PORT:
            _handle_update_port_request(driver, request, reply);
            break;
        case request_type::DELETE_PORT:
            _handle_delete_port_request(driver, request, reply);
            break;
        default:
            reply["code"] = reply_code::ERROR;
            reply["error"] = json_error(ENOSYS, "Request type not implemented in packtio port server");
        }

        std::vector<uint8_t> reply_buffer = json::to_cbor(reply);
        if ((send_or_err = zmq_send(data->socket, reply_buffer.data(), reply_buffer.size(), 0))
            != static_cast<int>(reply_buffer.size())) {
            ICP_LOG(ICP_LOG_ERROR, "Request reply failed: %s\n", zmq_strerror(errno));
        } else {
            ICP_LOG(ICP_LOG_TRACE, "Sent %s reply to %s request\n",
                    to_string(reply["code"].get<reply_code>()).c_str(),
                    to_string(type).c_str());
        }
    }

    zmq_msg_close(&request_msg);

    return (((recv_or_err < 0 || send_or_err < 0) && errno == ETERM) ? -1 : 0);
}

server::server(void* context,
               icp::core::event_loop& loop,
               generic_driver& driver)
    : m_socket(icp_socket_get_server(context, ZMQ_REP, endpoint.c_str()))
{
    struct icp_event_callbacks callbacks = {
        .on_read = _handle_rpc_request
    };
    loop.add(m_socket.get(), &callbacks, &driver);
}

}
}
}
}
