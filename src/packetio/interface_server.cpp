#include <memory>
#include <map>

#include <zmq.h>

#include "core/icp_core.h"
#include "packetio/api_server.h"
#include "packetio/interface_api.h"
#include "packetio/json_transmogrify.h"
#include "swagger/v1/model/Interface.h"

#include <iostream>

namespace icp {
namespace packetio {
namespace interface {
namespace api {

const std::string endpoint = "inproc://icp_packetio_interface";

using namespace swagger::v1::model;
using json = nlohmann::json;

typedef std::map<unsigned, std::shared_ptr<Interface>> interface_map;

const std::string & get_request_type(request_type type)
{
    const static std::unordered_map<request_type, std::string> request_types = {
        { request_type::LIST_INTERFACES,        "LIST_INTERFACES"        },
        { request_type::CREATE_INTERFACE,       "CREATE_INTERFACE"       },
        { request_type::GET_INTERFACE,          "GET_INTERFACE"          },
        { request_type::DELETE_INTERFACE,       "DELETE_INTERFACE"       },
        { request_type::BULK_CREATE_INTERFACES, "BULD_CREATE_INTERFACES" },
        { request_type::BULK_DELETE_INTERFACES, "BULD_DELETE_INTERFACES" },
        { request_type::NONE,                   "UNKNOWN"                }  /* Overloaded */
    };

    return (request_types.find(type) == request_types.end()
            ? request_types.at(request_type::NONE)
            : request_types.at(type));
}

const std::string & get_reply_code(reply_code code)
{
    const static std::unordered_map<reply_code, std::string> reply_codes = {
        { reply_code::OK,           "OK"           },
        { reply_code::NO_INTERFACE, "NO_INTERFACE" },
        { reply_code::BAD_INPUT,    "BAD_INPUT"    },
        { reply_code::ERROR,        "ERROR"        },
        { reply_code::NONE,         "UNKNOWN"      },  /* Overloaded */
    };

    return (reply_codes.find(code) == reply_codes.end()
            ? reply_codes.at(reply_code::NONE)
            : reply_codes.at(code));
}

static void _handle_list_interfaces_request(json &request, json& reply, interface_map& map)
{
    // TODO: filters
    (void)request;

    json jints = json::array();
    for (auto &item : map) {
        jints.push_back(item.second->toJson());
    }
    reply["code"] = reply_code::OK;
    reply["data"] = jints.dump();
}

static int interface_idx = 0;

static void _handle_create_interface_request(json& request, json&reply, interface_map& map)
{
    try {
        auto iface = std::make_shared<Interface>();
        *iface = json::parse(request["data"].get<std::string>());
        iface->setId(std::to_string(interface_idx));
        map[interface_idx++] = iface;

        reply["code"] = reply_code::OK;
        reply["data"] = iface->toJson().dump();
    } catch (const json::parse_error &e) {
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(e.id, e.what());
    }
}

static void _handle_get_interface_request(json& request, json& reply, interface_map& map)
{
    int id = request["id"].get<int>();
    auto item = map.find(id);
    if (item == map.end()) {
        reply["code"] = reply_code::NO_INTERFACE;
    } else {
        reply["code"] = reply_code::OK;
        reply["data"] = item->second->toJson().dump();
    }
}

static void _handle_delete_interface_request(json& request, json& reply, interface_map& map)
{
    int id = request["id"].get<int>();
    if (map.find(id) != map.end()) {
        map.erase(id);
    }
    reply["code"] = reply_code::OK;
}

static void _handle_bulk_create_interface_request(json& request, json& reply, interface_map& map)
{
    std::vector<int> ids;
    try {
        auto data = json::array();
        for (auto& item : request["items"]) {
            auto iface = std::make_shared<Interface>();
            *iface = item;
            iface->setId(std::to_string(interface_idx));
            ids.push_back(interface_idx);  /* store the id in case we encounter a subsequent error */
            map[interface_idx++] = iface;
            data.push_back(iface->toJson());
        }
        reply["code"] = reply_code::OK;
        reply["data"] = data.dump();
    } catch (const json::parse_error &e) {
        /* Delete any interfaces we successfully created */
        for (int id : ids) {
            if (map.find(id) != map.end()) {
                map.erase(id);
            }
        }
        reply["code"] = reply_code::BAD_INPUT;
        reply["error"] = json_error(e.id, e.what());
    }
}

static void _handle_bulk_delete_interface_request(json& request, json& reply, interface_map& map)
{
    for (int id : request["ids"]) {
        if (map.find(id) != map.end()) {
            map.erase(id);
        }
    }
    reply["code"] = reply_code::OK;
}

static int _handle_rpc_request(const icp_event_data *data, void *arg)
{
    interface_map *map = reinterpret_cast<interface_map *>(arg);

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
        request_type type = request.find("type") == request.end() ? request_type::NONE : request["type"].get<request_type>();
        json reply;

        switch (type) {
        case request_type::GET_INTERFACE:
        case request_type::DELETE_INTERFACE:
            icp_log(ICP_LOG_TRACE, "Received %s request for interface %d\n",
                    get_request_type(type).c_str(),
                    request["id"].get<int>());
            break;
        default:
            icp_log(ICP_LOG_TRACE, "Received %s request\n", get_request_type(type).c_str());
        }

        switch (type) {
        case request_type::LIST_INTERFACES:
            _handle_list_interfaces_request(request, reply, *map);
            break;
        case request_type::CREATE_INTERFACE:
            _handle_create_interface_request(request, reply, *map);
            break;
        case request_type::GET_INTERFACE:
            _handle_get_interface_request(request, reply, *map);
            break;
        case request_type::DELETE_INTERFACE:
            _handle_delete_interface_request(request, reply, *map);
            break;
        case request_type::BULK_CREATE_INTERFACES:
            _handle_bulk_create_interface_request(request, reply, *map);
            break;
        case request_type::BULK_DELETE_INTERFACES:
            _handle_bulk_delete_interface_request(request, reply, *map);
            break;
        default:
            reply["code"] = reply_code::ERROR;
            reply["error"] = json_error(ENOSYS, "Request type not implemented in packetio interface server");
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
        : _socket(icp_socket_get_server(context, ZMQ_REP, endpoint.c_str()))
    {
        struct icp_event_callbacks callbacks = {
            .on_read = _handle_rpc_request
        };
        loop.add(socket(), &callbacks, &interfaces());
    }

    interface_map& interfaces() { return _interfaces; }
    void* socket() { return _socket.get(); }

private:
    std::unique_ptr<void, icp_socket_deleter> _socket;
    interface_map _interfaces;
};

}
}
}
}
