#include <memory>

#include <json.hpp>
#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "api.hpp"

namespace opneperf::cpu {

using namespace Pistache;
using json = nlohmann::json;
namespace api = openperf::cpu::api;

class handler : public openperf::api::route::handler::registrar<handler>
{
    std::unique_ptr<void, op_socket_deleter> m_socket;

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

handler::handler(void* context, Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, api::endpoint.data()))
{
    Rest::Routes::Get(router,
                      "/cpu-generators",
                      Rest::Routes::bind(&handler::list_generators, this));
    Rest::Routes::Post(router,
                       "/cpu-generators",
                       Rest::Routes::bind(&handler::create_generator, this));
    Rest::Routes::Get(router,
                      "/cpu-generators/:id",
                      Rest::Routes::bind(&handler::get_generator, this));
    Rest::Routes::Delete(router,
                         "/cpu-generators/:id",
                         Rest::Routes::bind(&handler::delete_generator, this));
    Rest::Routes::Post(router,
                       "/cpu-generators/:id/start",
                       Rest::Routes::bind(&handler::start_generator, this));
    Rest::Routes::Post(router,
                       "/cpu-generators/:id/stop",
                       Rest::Routes::bind(&handler::stop_generator, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-start",
        Rest::Routes::bind(&handler::bulk_start_generators, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-stop",
        Rest::Routes::bind(&handler::bulk_stop_generators, this));
    Rest::Routes::Get(
        router,
        "/cpu-generator-results",
        Rest::Routes::bind(&handler::list_generator_results, this));
    Rest::Routes::Get(router,
                      "/cpu-generator-results/:id",
                      Rest::Routes::bind(&handler::get_generator_result, this));
    Rest::Routes::Delete(
        router,
        "/cpu-generator-results/:id",
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

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    /*auto api_reply =
        submit_request(m_socket.get(), api::request_block_generator_list{});
    if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto generators = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(generators),
                       [](const auto& blkgenerator) {
                           return (api::to_swagger(blkgenerator)->toJson());
                       });
        response.send(Http::Code::Ok, generators.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error);
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    /*try {
        auto generator_json = json::parse(request.body());
        BlockGenerator generator_model;
        generator_model.fromJson(generator_json);
        auto gc = BlockGeneratorConfig();
        gc.fromJson(generator_json["config"]);
        generator_model.setConfig(std::make_shared<BlockGeneratorConfig>(gc));

        auto api_reply =
            submit_request(m_socket.get(),
                           api::request_block_generator_add{
                               api::from_swagger(generator_model)});
        if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
            assert(!reply->generators.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(
                Http::Code::Ok,
                to_swagger(reply->generators.front())->toJson().dump());
        } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
            response.send(to_code(*error), api::to_string(error->info));
        } else {
            response.send(Http::Code::Internal_Server_Error);
        }
    } catch (const json::exception& e) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Bad_Request,
            nlohmann::json({{"code", e.id}, {"message", e.what()}}).dump());
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::get_generator(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_generator{id : id});
    if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Ok,
            api::to_swagger(reply->generators.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::No_Content);
        return;
    }
    auto api_reply = submit_request(m_socket.get(),
                                    api::request_block_generator_del{id : id});
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::No_Content);*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_start{id : id});
    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::stop_generator(const Rest::Request& request,
                             Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(),
                                    api::request_block_generator_stop{id : id});
    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    /*auto request_json = json::parse(request.body());
    BulkStartBlockGeneratorsRequest request_model;
    request_model.fromJson(request_json);

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (auto res =
            std::get_if<api::reply_block_generator_bulk_start>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(*res)->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    /*auto request_json = json::parse(request.body());
    BulkStopBlockGeneratorsRequest request_model;
    request_model.fromJson(request_json);

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::list_generator_results(const Rest::Request&,
                                     Http::ResponseWriter response)
{
    /*auto api_reply = submit_request(m_socket.get(),
                                    api::request_block_generator_result_list{});
    if (auto reply =
            std::get_if<api::reply_block_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto results = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return (api::to_swagger(result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error);
}

void handler::get_generator_result(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_result{id : id});
    if (auto reply =
            std::get_if<api::reply_block_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      api::to_swagger(reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

void handler::delete_generator_result(const Rest::Request& request,
                                      Http::ResponseWriter response)
{
    /*auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::No_Content);
        return;
    }
    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_result_del{id : id});
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::No_Content);*/
    response.send(Http::Code::Internal_Server_Error, request.body());
}

} // namespace opneperf::cpu