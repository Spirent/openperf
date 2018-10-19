#include <zmq.h>

#include "api/api_route_handler.h"
#include "core/icp_core.h"
#include "packetio/port_api.h"

namespace icp {
namespace packetio {
namespace port {
namespace api {

using namespace Pistache;
using json = nlohmann::json;

json submit_request(void *socket, json& request)
{
    auto type = request["type"].get<request_type>();
    switch (type) {
    case request_type::GET_PORT:
    case request_type::UPDATE_PORT:
    case request_type::DELETE_PORT:
        icp_log(ICP_LOG_TRACE, "Sending %s request for port %d\n",
                to_string(type).c_str(),
                request["id"].get<int>());
        break;
    default:
        icp_log(ICP_LOG_TRACE, "Sending %s request\n", to_string(type).c_str());
    }

    std::vector<uint8_t> request_buffer = json::to_cbor(request);
    zmq_msg_t reply_msg;
    if (zmq_msg_init(&reply_msg) == -1
        || zmq_send(socket, request_buffer.data(), request_buffer.size(), 0) != static_cast<int>(request_buffer.size())
        || zmq_msg_recv(&reply_msg, socket, 0) == -1) {
        return {
            { "code", reply_code::ERROR },
            { "error", json_error(errno, zmq_strerror(errno)) }
        };
    }

    icp_log(ICP_LOG_TRACE, "Received %s reply\n", to_string(type).c_str());

    std::vector<uint8_t> reply_buffer(static_cast<uint8_t *>(zmq_msg_data(&reply_msg)),
                                      static_cast<uint8_t *>(zmq_msg_data(&reply_msg)) + zmq_msg_size(&reply_msg));

    zmq_msg_close(&reply_msg);

    return json::from_cbor(reply_buffer);
}

class handler : public icp::api::route::handler::registrar<handler> {
public:
    handler(void *context, Rest::Router &router);

    void  list_ports(const Rest::Request &request, Http::ResponseWriter response);
    void create_port(const Rest::Request &request, Http::ResponseWriter response);
    void    get_port(const Rest::Request &request, Http::ResponseWriter response);
    void update_port(const Rest::Request &request, Http::ResponseWriter response);
    void delete_port(const Rest::Request &request, Http::ResponseWriter response);

private:
    std::unique_ptr<void, icp_socket_deleter> socket;
};

handler::handler(void *context, Rest::Router &router)
    : socket(icp_socket_get_client(context, ZMQ_REQ, endpoint.c_str()))
{
    Rest::Routes::Get(router, "/ports",
                      Rest::Routes::bind(&handler::list_ports, this));
    Rest::Routes::Post(router, "/ports",
                       Rest::Routes::bind(&handler::create_port, this));
    Rest::Routes::Get(router, "/ports/:id",
                      Rest::Routes::bind(&handler::get_port, this));
    Rest::Routes::Put(router, "/ports/:id",
                      Rest::Routes::bind(&handler::update_port, this));
    Rest::Routes::Delete(router, "/ports/:id",
                         Rest::Routes::bind(&handler::delete_port, this));
}

void handler::list_ports(const Rest::Request &request, Http::ResponseWriter response)
{
    json api_request = {
        {"type", request_type::LIST_PORTS }
    };

    if (request.query().has("kind")) {
        api_request["kind"] = request.query().get("kind").get();
    }

    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<reply_code>() == reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
    } else {
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::create_port(const Rest::Request &request, Http::ResponseWriter response)
{
    json api_request = {
        { "type", request_type::CREATE_PORT },
        { "data", request.body() }
    };

    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.send(Http::Code::Ok,
                      api_reply["data"].get<std::string>());
        break;
    case reply_code::BAD_INPUT:
        response.send(Http::Code::Bad_Request,
                      api_reply["error"].get<std::string>());
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

/**
 * Our id might not be an int.  In that case, the Pistache request will throw
 * an exception.  This macro handles catching the exception and returning
 * the appropriate response.  Unfortunately, we cannot make this a function
 * because the response is an unmovable object.
 */

#define SAFE_GET_ID(id_, response_, code_)                              \
    do {                                                                \
        try {                                                           \
            id_ = request.param(":id").as<int>();                       \
        } catch (const std::runtime_error&) {                           \
            response_.send(code_);                                      \
            return;                                                     \
        }                                                               \
    } while (0)

void handler::get_port(const Rest::Request &request, Http::ResponseWriter response)
{
    int id = -1;
    SAFE_GET_ID(id, response, Http::Code::Not_Found);

    json api_request = {
        {"type", request_type::GET_PORT},
        {"id", id }
    };

    json api_reply = submit_request(socket.get(), api_request);

    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok,
                      api_reply["data"].get<std::string>());
        break;
    case reply_code::NO_PORT:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::update_port(const Rest::Request &request, Http::ResponseWriter response)
{
    int id = -1;
    SAFE_GET_ID(id, response, Http::Code::Not_Found);

    json api_request = {
        { "type", request_type::UPDATE_PORT },
        { "id", id },
        { "data", request.body() }
    };

    json api_reply = submit_request(socket.get(), api_request);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    switch(api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok,
                      api_reply["data"].get<std::string>());
        break;
    case reply_code::NO_PORT:
        response.send(Http::Code::Not_Found);
        break;
    case reply_code::BAD_INPUT:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Bad_Request,
                      api_reply["error"].get<std::string>());
        break;
    default:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::delete_port(const Rest::Request &request, Http::ResponseWriter response)
{
    int id = -1;
    SAFE_GET_ID(id, response, Http::Code::No_Content);

    json api_request = {
        { "type", request_type::DELETE_PORT },
        { "id", request.param(":id").as<int>() }
    };

    /* We don't care about any reply, here */
    json api_reply = submit_request(socket.get(), api_request);
    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.send(Http::Code::No_Content);
        break;
    case reply_code::BAD_INPUT:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Bad_Request,
                      api_reply["error"].get<std::string>());
        break;
    default:
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

}
}
}
}
