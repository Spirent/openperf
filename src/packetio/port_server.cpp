#include <unordered_map>

#include "zmq.h"

#include "core/icp_core.h"
#include "packetio/api_server.h"
#include "packetio/port_api.h"
#include "swagger/v1/model/Port.h"

namespace icp {
namespace packetio {
namespace port {
namespace api {

const std::string endpoint = "inproc://icp_packetio_port";

using namespace swagger::v1::model;
using json = nlohmann::json;

const std::string & get_request_type(request_type type)
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

const std::string & get_reply_code(reply_code code)
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
static T get_json_key(json& j, const char * key, T default_value)
{
    return (j.find(key) == j.end() ? default_value : j[key].get<T>());
}

static void _handle_list_ports_request(json& request, json& reply)
{
    std::vector<std::shared_ptr<Port>> ports;
    auto kind = get_json_key<std::string>(request, "kind", "");
    impl::list_ports(ports, kind);
    json jports = json::array();
    for (auto &port : ports) {
        jports.push_back(port->toJson());
    }
    reply["code"] = reply_code::OK;
    reply["data"] = jports.dump();
}

static void _handle_create_port_request(json& request, json& reply)
{
    // TODO
}

static void _handle_get_port_request(json& request, json& reply)
{
    int port_idx = request["port_idx"].get<int>();
    if (!impl::is_valid_port(port_idx)) {
        reply["code"] = reply_code::NO_PORT;
    } else {
        auto port = impl::get_port(port_idx);
        reply["code"] = reply_code::OK;
        reply["data"] = port->toJson().dump();
    }
}

static void _handle_update_port_request(json& request, json& reply)
{
    int port_idx = request["port_idx"].get<int>();
    if (!impl::is_valid_port(port_idx)) {
        reply["code"] = reply_code::NO_PORT;
        return;
    }

    auto port = impl::get_port(port_idx);

    try {
        auto in_port = json::parse(request["data"].get<std::string>());

        if (in_port.find("config") != in_port.end()) {
            auto in_config = json::parse(in_port["config"].get<std::string>());
            if (in_config.find("dpdk") != in_config.end()
                && port->getKind() == "dpdk") {
                // TODO: Update dpdk config
            } else if (in_config.find("bond") != in_config.end()
                       && port->getKind() == "bond") {
                // TODO: Update bond config
            }
        }

        /* Reload port data for reply */
        reply["code"] = reply_code::OK;
        reply["data"] = impl::get_port(port_idx)->toJson().dump();

    } catch (const json::parse_error &e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(e.id, e.what());
    }
}

static void _handle_delete_port_request(json & request, json& reply)
{
    impl::delete_port(request["port_idx"].get<int>());
    reply["code"] = reply_code::OK;
}

static int _handle_rpc_request(const icp_event_data *data,
                               void *arg __attribute__((unused)))
{
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
        request_type type = get_json_key(request, "type", request_type::NONE);
        json reply;

        switch (type) {
        case request_type::GET_PORT:
        case request_type::UPDATE_PORT:
        case request_type::DELETE_PORT:
            icp_log(ICP_LOG_TRACE, "Received %s request for port %d\n",
                    get_request_type(type).c_str(),
                    request["port_idx"].get<int>());
            break;
        default:
            icp_log(ICP_LOG_TRACE, "Received %s request\n", get_request_type(type).c_str());
        }

        switch (type) {
        case request_type::LIST_PORTS:
            _handle_list_ports_request(request, reply);
            break;
        case request_type::CREATE_PORT:
            _handle_create_port_request(request, reply);
            break;
        case request_type::GET_PORT:
            _handle_get_port_request(request, reply);
            break;
        case request_type::UPDATE_PORT:
            _handle_update_port_request(request, reply);
            break;
        case request_type::DELETE_PORT:
            _handle_delete_port_request(request, reply);
            break;
        default:
            reply["code"] = reply_code::ERROR;
            reply["data"] = "Unsupported request type, " + std::to_string(static_cast<int>(type));
        }

        std::vector<uint8_t> reply_buffer = json::to_cbor(reply);
        if ((send_or_err = zmq_send(data->socket, reply_buffer.data(), reply_buffer.size(), 0))
            != static_cast<int>(reply_buffer.size())) {
            icp_log(ICP_LOG_ERROR, "Request reply failed: %s\n", zmq_strerror(errno));
        } else {
            icp_log(ICP_LOG_TRACE, "Sent %s reply to %s request\n",
                    get_reply_code(reply["code"]).c_str(),
                    get_request_type(type).c_str());
        }
    }

    zmq_msg_close(&request_msg);

    return (((recv_or_err < 0 || send_or_err < 0) && errno == ETERM) ? -1 : 0);
}

class server : public icp::packetio::api::server::registrar<server> {
public:
    server(void *context, icp::core::event_loop &loop)
        : socket(icp_socket_get_server(context, ZMQ_REP, endpoint.c_str()))
    {
        struct icp_event_callbacks callbacks = {
            .on_read = _handle_rpc_request
        };
        loop.add(socket.get(), &callbacks, nullptr);
    }
private:
    std::unique_ptr<void, icp_socket_deleter> socket;
};

}
}
}
}
