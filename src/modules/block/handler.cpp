#include <memory>

#include <json.hpp>
#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "block/api.hpp"
#include "swagger/v1/model/BlockFile.h"

namespace opneperf::block {

using namespace Pistache;
using json = nlohmann::json;
using namespace swagger::v1::model;
namespace api = openperf::block::api;

class handler : public openperf::api::route::handler::registrar<handler>
{
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    handler(void* context, Rest::Router& router);

    void list_devices(const Rest::Request& request,
                      Http::ResponseWriter response);

    void get_device(const Rest::Request& request,
                    Http::ResponseWriter response);

    void list_files(const Rest::Request& request,
                    Http::ResponseWriter response);

    void create_file(const Rest::Request& request,
                     Http::ResponseWriter response);

    void get_file(const Rest::Request& request, Http::ResponseWriter response);

    void delete_file(const Rest::Request& request,
                     Http::ResponseWriter response);

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

    void bulk_start_generators(const Rest::Request& request,
                               Http::ResponseWriter response);

    void bulk_stop_generators(const Rest::Request& request,
                              Http::ResponseWriter response);

    void list_generator_results(const Rest::Request& request,
                               Http::ResponseWriter response);

    void get_generator_result(const Rest::Request& request,
                              Http::ResponseWriter response);

    void delete_generator_result(const Rest::Request& request,
                              Http::ResponseWriter response);
};

enum Http::Code to_code(const api::reply_error& error)
{
    switch (error.info.type) {
    case api::error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

const char* to_string(const api::reply_error& error)
{
    switch (error.info.type) {
    case api::error_type::NOT_FOUND:
        return ("");
    case api::error_type::ZMQ_ERROR:
        return (zmq_strerror(error.info.value));
    default:
        return ("unknown error type");
    }
}

json submit_request(void* socket, json& request)
{
    auto type = request["type"].get<api::request_type>();
    switch (type) {
    case api::request_type::LIST_DEVICES:
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
        return {{"code", api::reply_code::ERROR},
                {"error", api::json_error(errno, zmq_strerror(errno))}};
    }

    OP_LOG(OP_LOG_TRACE, "Received %s reply\n", to_string(type).c_str());

    std::vector<uint8_t> reply_buffer(
        static_cast<uint8_t*>(zmq_msg_data(&reply_msg)),
        static_cast<uint8_t*>(zmq_msg_data(&reply_msg))
            + zmq_msg_size(&reply_msg));

    zmq_msg_close(&reply_msg);

    return json::from_cbor(reply_buffer);
}

handler::handler(void* context, Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, api::endpoint.data()))
{
    /* Time counter handlers */
    Rest::Routes::Get(router,
                      "/block-devices",
                      Rest::Routes::bind(&handler::list_devices, this));
    Rest::Routes::Get(router,
                      "/block-devices/:id",
                      Rest::Routes::bind(&handler::get_device, this));
    Rest::Routes::Get(
        router, "/block-files", Rest::Routes::bind(&handler::list_files, this));
    Rest::Routes::Post(router,
                       "/block-files",
                       Rest::Routes::bind(&handler::create_file, this));
    Rest::Routes::Get(router,
                      "/block-files/:id",
                      Rest::Routes::bind(&handler::get_file, this));
    Rest::Routes::Delete(router,
                         "/block-files/:id",
                         Rest::Routes::bind(&handler::delete_file, this));
    Rest::Routes::Get(router,
                      "/block-generators",
                      Rest::Routes::bind(&handler::list_generators, this));
    Rest::Routes::Post(router,
                       "/block-generators",
                       Rest::Routes::bind(&handler::create_generator, this));
    Rest::Routes::Get(router,
                      "/block-generators/:id",
                      Rest::Routes::bind(&handler::get_generator, this));
    Rest::Routes::Delete(router,
                         "/block-generators/:id",
                         Rest::Routes::bind(&handler::delete_generator, this));
    Rest::Routes::Post(router,
                       "/block-generators/:id/start",
                       Rest::Routes::bind(&handler::start_generator, this));
    Rest::Routes::Post(router,
                       "/block-generators/:id/stop",
                       Rest::Routes::bind(&handler::stop_generator, this));
    Rest::Routes::Post(
        router,
        "/block-generators/x/bulk-start",
        Rest::Routes::bind(&handler::bulk_start_generators, this));
    Rest::Routes::Post(
        router,
        "/block-generators/x/bulk-stop",
        Rest::Routes::bind(&handler::bulk_stop_generators, this));
    Rest::Routes::Get(router,
                      "/block-generator-results",
                      Rest::Routes::bind(&handler::list_generator_results, this));
    Rest::Routes::Get(router,
                      "/block-generator-results/:id",
                      Rest::Routes::bind(&handler::get_generator_result, this));
    Rest::Routes::Delete(router,
                         "/block-generator-results/:id",
                         Rest::Routes::bind(&handler::delete_generator_result, this));
}

api::reply_msg submit_request(void* socket, const api::request_msg& request)
{
    if (auto error = api::send_message(socket, api::serialize_request(request));
        error != 0) {
        return (to_error(api::error_type::ZMQ_ERROR, error));
    }
    auto reply = api::recv_message(socket).and_then(api::deserialize_reply);
    if (!reply) {
        return (to_error(api::error_type::ZMQ_ERROR, reply.error()));
    }

    return (*reply);
}

void handler::list_devices(const Rest::Request&, Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_block_device_list{});
    if (auto reply = std::get_if<api::reply_block_devices>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto devices = json::array();
        std::transform(std::begin(reply->devices),
                       std::end(reply->devices),
                       std::back_inserter(devices),
                       [](const auto& device) {
                           return (api::to_swagger(device)->toJson());
                       });
        response.send(Http::Code::Ok, devices.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::get_device(const Rest::Request& request,
                         Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_device{id: id});
    if (auto reply = std::get_if<api::reply_block_devices>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(reply->devices.at(0))->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::list_files(const Rest::Request&, Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_block_file_list{});
    if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto files = json::array();
        std::transform(std::begin(reply->files),
                       std::end(reply->files),
                       std::back_inserter(files),
                       [](const auto& blkfile) {
                           return (api::to_swagger(blkfile)->toJson());
                       });
        response.send(Http::Code::Ok, files.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_file(const Rest::Request& request,
                          Http::ResponseWriter response)
{
    try {
        auto file_json = json::parse(request.body());
        BlockFile file_model;
        file_model.fromJson(file_json);

        auto api_reply =
            submit_request(m_socket.get(), api::request_block_file_add{api::from_swagger(file_model)});
        if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
            assert(!reply->files.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(Http::Code::Ok,
                          to_swagger(reply->files.front())->toJson().dump());
        } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
            response.send(to_code(*error), to_string(*error));
        } else {
            response.send(Http::Code::Internal_Server_Error);
        }
    } catch (const json::exception& e) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Bad_Request,
            nlohmann::json({{"code", e.id}, {"message", e.what()}}).dump());
    }
}

void handler::get_file(const Rest::Request& request,
                       Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_file{id: id});
    if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(reply->files.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_file(const Rest::Request& request,
                          Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_file_del{id: id});
    response.headers().add<Http::Header::ContentType>(
        MIME(Application, Json));
    response.send(Http::Code::No_Content);
}

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    json api_request = {{"type", api::request_type::LIST_GENERATORS}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<api::reply_code>() == api::reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
    } else {
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    json api_request = {{"type", api::request_type::CREATE_GENERATOR},
                        {"data", request.body()}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case api::reply_code::BAD_INPUT:
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
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", api::request_type::GET_GENERATOR}, {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case api::reply_code::NO_FILE:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", api::request_type::DELETE_GENERATOR},
                        {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<api::reply_code>() == api::reply_code::OK) {
        response.send(Http::Code::No_Content);
    } else {
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }
    json api_request = {{"type", api::request_type::START_GENERATOR}, {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::No_Content);
        break;
    case api::reply_code::NO_GENERATOR:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::stop_generator(const Rest::Request& request,
                             Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", api::request_type::STOP_GENERATOR}, {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::No_Content);
        break;
    case api::reply_code::NO_GENERATOR:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    json api_request = {{"type", api::request_type::BULK_START_GENERATORS},
                        {"data", request.body()}};

    json api_reply = submit_request(m_socket.get(), api_request);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case api::reply_code::BAD_INPUT:
        response.send(Http::Code::Bad_Request,
                      api_reply["error"].get<std::string>());
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    json api_request = {{"type", api::request_type::BULK_STOP_GENERATORS},
                        {"data", request.body()}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::No_Content);
        break;
    case api::reply_code::BAD_INPUT:
        response.send(Http::Code::Bad_Request,
                      api_reply["error"].get<std::string>());
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::list_generator_results(const Rest::Request&,
                              Http::ResponseWriter response)
{
    json api_request = {{"type", api::request_type::LIST_GENERATOR_RESULTS}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<api::reply_code>() == api::reply_code::OK) {
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
    } else {
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::get_generator_result(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", api::request_type::GET_GENERATOR_RESULT}, {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));

    switch (api_reply["code"].get<api::reply_code>()) {
    case api::reply_code::OK:
        response.send(Http::Code::Ok, api_reply["data"].get<std::string>());
        break;
    case api::reply_code::NO_FILE:
        response.send(Http::Code::Not_Found);
        break;
    default:
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}

void handler::delete_generator_result(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    json api_request = {{"type", api::request_type::DELETE_GENERATOR_RESULT},
                        {"id", id}};

    json api_reply = submit_request(m_socket.get(), api_request);

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    if (api_reply["code"].get<api::reply_code>() == api::reply_code::OK) {
        response.send(Http::Code::No_Content);
    } else {
        response.send(Http::Code::Internal_Server_Error,
                      api_reply["error"].get<std::string>());
    }
}


} // namespace opneperf::block
