#include <zmq.h>
#include <json.hpp>

#include "api/api_route_handler.hpp"
#include "memory/api.hpp"

#include "core/op_core.h"
#include "config/op_config_utils.hpp"

namespace openperf::memory::api {

using json = nlohmann::json;
using namespace Pistache;

json submit_request(void* socket, json& request)
{
    auto type = request["type"].get<request_type>();

    switch (type) {
    case request_type::GET_GENERATOR:
    case request_type::DELETE_GENERATOR:
        OP_LOG(OP_LOG_TRACE,
               "Sending %s request for interface %s\n",
               to_string(type).c_str(),
               request["id"].get<std::string>().c_str());
        break;
    default:
        OP_LOG(OP_LOG_TRACE, "Sending %s request\n", to_string(type).c_str());
    }

    std::vector<uint8_t> request_buffer = json::to_cbor(request);
    zmq_msg_t reply_msg;
    if (zmq_msg_init(&reply_msg) == -1
        || zmq_send(socket, request_buffer.data(), request_buffer.size(), 0)
               != static_cast<int>(request_buffer.size())
        || zmq_msg_recv(&reply_msg, socket, 0) == -1) {
        return {{"code", reply_code::ERROR},
                {"error", json_error(errno, zmq_strerror(errno))}};
    }

    OP_LOG(OP_LOG_TRACE, "Received %s reply\n", to_string(type).c_str());

    std::vector<uint8_t> reply_buffer(
        static_cast<uint8_t*>(zmq_msg_data(&reply_msg)),
        static_cast<uint8_t*>(zmq_msg_data(&reply_msg))
            + zmq_msg_size(&reply_msg));

    zmq_msg_close(&reply_msg);

    return json::from_cbor(reply_buffer);
}

class handler : public openperf::api::route::handler::registrar<handler>
{
private:
    std::unique_ptr<void, op_socket_deleter> socket;

public:
    handler(void* context, Rest::Router& router);

    void list_generators(const Rest::Request& request,
                         Http::ResponseWriter response);
    void create_generator(const Rest::Request& request,
                          Http::ResponseWriter response);
    void get_generator(const Rest::Request& request,
                       Http::ResponseWriter response);
    void delete_generator(const Rest::Request& request,
                          Http::ResponseWriter response);
    void start_generator(const Rest::Request& request,
                         Http::ResponseWriter response);
    void stop_generator(const Rest::Request& request,
                        Http::ResponseWriter response);

    // Bulk memory generator actions
    void bulk_start_generators(const Rest::Request& request,
                               Http::ResponseWriter response);
    void bulk_stop_generators(const Rest::Request& request,
                              Http::ResponseWriter response);

    // Memory generator results
    void list_results(const Rest::Request& request,
                      Http::ResponseWriter response);
    void get_result(const Rest::Request& request,
                    Http::ResponseWriter response);
    void delete_result(const Rest::Request& request,
                       Http::ResponseWriter response);

    void get_info(const Rest::Request& request, Http::ResponseWriter response);
};

handler::handler(void* context, Rest::Router& router)
    : socket(op_socket_get_client(
          context, ZMQ_REQ, openperf::memory::api::endpoint.data()))
{
    Rest::Routes::Get(router,
                      "/memory-generators",
                      Rest::Routes::bind(&handler::list_generators, this));
    Rest::Routes::Post(router,
                       "/memory-generators",
                       Rest::Routes::bind(&handler::create_generator, this));
    Rest::Routes::Get(router,
                      "/memory-generators/:id",
                      Rest::Routes::bind(&handler::get_generator, this));
    Rest::Routes::Delete(router,
                         "/memory-generators/:id",
                         Rest::Routes::bind(&handler::delete_generator, this));
    Rest::Routes::Post(router,
                       "/memory-generators/:id/start",
                       Rest::Routes::bind(&handler::start_generator, this));
    Rest::Routes::Post(router,
                       "/memory-generators/:id/stop",
                       Rest::Routes::bind(&handler::stop_generator, this));

    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-start",
        Rest::Routes::bind(&handler::bulk_start_generators, this));
    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-stop",
        Rest::Routes::bind(&handler::bulk_stop_generators, this));

    Rest::Routes::Get(router,
                      "/memory-generator-results",
                      Rest::Routes::bind(&handler::list_results, this));
    Rest::Routes::Get(router,
                      "/memory-generator-results/:id",
                      Rest::Routes::bind(&handler::get_result, this));
    Rest::Routes::Delete(router,
                         "/memory-generator-results/:id",
                         Rest::Routes::bind(&handler::delete_result, this));

    Rest::Routes::Get(
        router, "/memory-info", Rest::Routes::bind(&handler::get_info, this));
}

void handler::list_generators(const Rest::Request& /*request*/,
                              Http::ResponseWriter response)
{
    json api_request = {{"type", request_type::LIST_GENERATORS}};
    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<reply_code>() == reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        return;
    }

    response.send(Http::Code::Internal_Server_Error,
                  api_reply["error"].get<std::string>());
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    json api_request = {{"type", request_type::CREATE_GENERATOR},
                        {"data", request.body()}};

    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
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

void handler::get_generator(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::GET_GENERATOR}, {"id", id}};

    json api_reply = submit_request(socket.get(), api_request);
    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case reply_code::NO_GENERATOR:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::DELETE_GENERATOR}, {"id", id}};

    submit_request(socket.get(), api_request);
    response.send(Http::Code::No_Content);
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::START_GENERATOR}, {"id", id}};

    submit_request(socket.get(), api_request);
    response.send(Http::Code::No_Content);
}

void handler::stop_generator(const Rest::Request& request,
                             Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::STOP_GENERATOR}, {"id", id}};

    submit_request(socket.get(), api_request);
    response.send(Http::Code::No_Content);
}

// Bulk memory generator actions
void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    try {
        json body = json::parse(request.body());
        if (body.find("ids") != body.end()) {
            json api_request = {{"type", request_type::BULK_START_GENERATORS},
                                {"ids", body["ids"]}};

            submit_request(socket.get(), api_request);
            response.send(Http::Code::No_Content);
        }
    } catch (const json::parse_error& e) {
        response.send(Http::Code::Bad_Request, json_error(e.id, e.what()));
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    try {
        json body = json::parse(request.body());
        if (body.find("ids") != body.end()) {
            json api_request = {{"type", request_type::BULK_STOP_GENERATORS},
                                {"ids", body["ids"]}};

            submit_request(socket.get(), api_request);
            response.send(Http::Code::No_Content);
        }
    } catch (const json::parse_error& e) {
        response.send(Http::Code::Bad_Request, json_error(e.id, e.what()));
    }
}

// Memory generator results
void handler::list_results(const Rest::Request& /*request*/,
                           Http::ResponseWriter response)
{
    json api_request = {{"type", request_type::LIST_RESULTS}};
    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<reply_code>() == reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        return;
    }

    response.send(Http::Code::Internal_Server_Error,
                  api_reply["error"].get<std::string>());
}

void handler::get_result(const Rest::Request& request,
                         Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::GET_RESULT}, {"id", id}};

    json api_reply = submit_request(socket.get(), api_request);
    switch (api_reply["code"].get<reply_code>()) {
    case reply_code::OK:
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case reply_code::NO_RESULT:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::delete_result(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", request_type::DELETE_RESULT}, {"id", id}};

    submit_request(socket.get(), api_request);
    response.send(Http::Code::No_Content);
}

void handler::get_info(const Rest::Request& /*request*/,
                       Http::ResponseWriter response)
{
    json api_request = {{"type", request_type::GET_INFO}};
    json api_reply = submit_request(socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<reply_code>() == reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        return;
    }

    response.send(Http::Code::Internal_Server_Error,
                  api_reply["error"].get<std::string>());
}

} // namespace openperf::memory::api
