#include <memory>

#include <json.hpp>
#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "api.hpp"
#include "swagger/v1/model/CpuGenerator.h"

namespace opneperf::cpu {

using namespace Pistache;
using namespace swagger::v1::model;
using json = nlohmann::json;
using request_type = Pistache::Rest::Request;
using response_type = Pistache::Http::ResponseWriter;

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

    void get_cpu_info(const Rest::Request& request,
                                 Http::ResponseWriter response);
};

enum Http::Code to_code(const api::reply_error& error)
{
    switch (error.info->type) {
    case api::error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case api::error_type::CUSTOM_ERROR:
        return (Http::Code::Bad_Request);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

static std::optional<std::string>
maybe_get_host_uri(const request_type& request)
{
    if (request.headers().has<Http::Header::Host>()) {
        auto host_header = request.headers().get<Http::Header::Host>();
        return ("http://" + host_header->host() + ":" + host_header->port().toString());
    }

    return (std::nullopt);
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
    Rest::Routes::Get(router, "/cpu-info", Rest::Routes::bind(&handler::get_cpu_info, this));
}

api::reply_msg submit_request(void* socket, api::request_msg&& request)
{
    if (auto error = api::send_message(socket, api::serialize_request(std::forward<api::request_msg>(request)));
        error != 0) {
        return (to_error(api::error_type::ZMQ_ERROR, error));
    }
    auto reply = api::recv_message(socket).and_then(api::deserialize_reply);
    if (!reply) {
        return (to_error(api::error_type::ZMQ_ERROR, reply.error()));
    }
    return (std::move(*reply));
}

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_list{});
    if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto generators = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(generators),
                       [](const auto& generator) {
                           return (api::to_swagger(*generator)->toJson());
                       });
        response.send(Http::Code::Ok, generators.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    try {
        auto generator_json = json::parse(request.body());

        CpuGeneratorConfig gc;
        gc.fromJson(generator_json["config"]);

        CpuGenerator generator_model;
        generator_model.fromJson(generator_json);
        generator_model.setConfig(std::make_shared<CpuGeneratorConfig>(gc));

        auto api_request = api::request_cpu_generator_add{
            std::make_unique<api::cpu_generator_t>(api::from_swagger(generator_model))
        };
        auto api_reply =
            submit_request(m_socket.get(), std::move(api_request));
        if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
            assert(!reply->generators.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            if (auto uri = maybe_get_host_uri(request); uri.has_value()) {
                response.headers().add<Http::Header::Location>(*uri + request.resource() + "/" + reply->generators.front()->id());
            }
            response.send(Http::Code::Created, api::to_swagger(*reply->generators.front())->toJson().dump());
        } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
            response.send(to_code(*error), api::to_string(*error->info));
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

void handler::get_generator(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator{id});
    if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
         assert(!reply->generators.empty());
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(*reply->generators.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    submit_request(m_socket.get(), api::request_cpu_generator_del{id});
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::No_Content);
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_start{id});
    if (auto reply = std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        assert(!reply->results.empty());
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        if (auto uri = maybe_get_host_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + "/cpu-generator-results/"
                + reply->results.front()->id());
        }
        response.send(Http::Code::Created, api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::stop_generator(const Rest::Request& request,
                             Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_stop{id});
    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    auto request_json = json::parse(request.body());
    BulkStartCpuGeneratorsRequest request_model;
    request_model.fromJson(request_json);

    api::request_cpu_generator_bulk_start bulk_request{
        std::make_unique<std::vector<std::string>>()
    };
    for (auto & id : request_model.getIds()) {
        if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
            response.send(Http::Code::Bad_Request, res.error());
            return;
        }
        bulk_request.ids->push_back(id);
    }

    auto api_reply =
        submit_request(m_socket.get(), std::move(bulk_request));
    if (auto reply =
            std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto results = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return (api::to_swagger(*result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto request_json = json::parse(request.body());
    BulkStopCpuGeneratorsRequest request_model;
    request_model.fromJson(request_json);

    api::request_cpu_generator_bulk_stop bulk_request{
        std::make_unique<std::vector<std::string>>()
    };
    for (auto & id : request_model.getIds()) {
        if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
            response.send(Http::Code::Bad_Request, res.error());
            return;
        }
        bulk_request.ids->push_back(id);
    }

    auto api_reply =
        submit_request(m_socket.get(), std::move(bulk_request));
    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::list_generator_results(const Rest::Request&,
                                     Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_result_list{});
    if (auto reply = std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto results = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return (api::to_swagger(*result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::get_generator_result(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_result{id});
    if (auto reply = std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
         assert(!reply->results.empty());
        response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_generator_result(const Rest::Request& request,
                                      Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    submit_request(m_socket.get(), api::request_cpu_generator_result_del{id});
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::No_Content);
}

void handler::get_cpu_info(const Rest::Request&, Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_info{});
    if (auto reply = std::get_if<api::reply_cpu_info>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, api::to_swagger(*reply->info)->toJson().dump());
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

} // namespace opneperf::cpu
