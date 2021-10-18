#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "api/api_utils.hpp"
#include "cpu/api.hpp"
#include "dynamic/api.hpp"
#include "framework/config/op_config_utils.hpp"
#include "framework/core/op_core.h"
#include "framework/message/serialized_message.hpp"

#include "swagger/converters/cpu.hpp"
#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"
#include "swagger/v1/model/CpuInfoResult.h"
#include "swagger/v1/model/BulkDeleteCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartCpuGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopCpuGeneratorsRequest.h"

namespace openperf::cpu::api {

using namespace Pistache;
using namespace swagger::v1::model;
using json = nlohmann::json;

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

    void bulk_create_generators(const Rest::Request& request,
                                Http::ResponseWriter response);

    void bulk_delete_generators(const Rest::Request& request,
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

static enum Http::Code to_code(const api::reply_error& error)
{
    switch (error.info.type) {
    case api::error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case api::error_type::POSIX:
        return (Http::Code::Bad_Request);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

static void handle_reply_error(const api::reply_msg& reply,
                               Pistache::Http::ResponseWriter response)
{
    if (auto error = std::get_if<api::reply_error>(&reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

static std::string concatenate(const std::vector<std::string>& strings)
{
    return (std::accumulate(
        std::begin(strings),
        std::end(strings),
        std::string{},
        [](std::string& lhs, const std::string& rhs) -> decltype(auto) {
            return (lhs += ((lhs.empty() ? "" : " ") + rhs));
        }));
}

static std::string json_error(int code, std::string_view message)
{
    return (
        nlohmann::json({{"code", code}, {"message", message.data()}}).dump());
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
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-create",
        Rest::Routes::bind(&handler::bulk_create_generators, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-delete",
        Rest::Routes::bind(&handler::bulk_delete_generators, this));
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

    Rest::Routes::Get(
        router, "/cpu-info", Rest::Routes::bind(&handler::get_cpu_info, this));
}

api::reply_msg submit_request(void* socket, api::request_msg&& request)
{
    if (auto error = openperf::message::send(
            socket,
            api::serialize_request(std::forward<api::request_msg>(request)));
        error != 0) {
        return to_error(api::error_type::ZMQ_ERROR, error);
    }
    auto reply =
        openperf::message::recv(socket).and_then(api::deserialize_reply);
    if (!reply) { return to_error(api::error_type::ZMQ_ERROR, reply.error()); }
    return std::move(*reply);
}

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_list{});
    if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
        auto generators = json::array();
        std::transform(
            std::begin(reply->generators),
            std::end(reply->generators),
            std::back_inserter(generators),
            [](const auto& generator) { return (generator->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, generators);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<swagger::v1::model::CpuGenerator, std::string>
parse_create_generator(const Pistache::Rest::Request& request)
{
    try {
        return (json::parse(request.body())
                    .get<swagger::v1::model::CpuGenerator>());
    } catch (const json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto generator = parse_create_generator(request);
    if (!generator) {
        response.send(Http::Code::Bad_Request, generator.error());
        return;
    }

    /* Validate any user provided id before forwarding request */
    if (!generator->getId().empty()) {
        auto res = config::op_config_validate_id_string(generator->getId());
        if (!res) {
            response.send(Http::Code::Bad_Request, res.error());
            return;
        }
    }

    /* Make sure generator object is valid before forwarding to the server */
    std::vector<std::string> errors;
    if (!is_valid(*generator, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_request = api::request_cpu_generator_add{
        std::make_unique<swagger::v1::model::CpuGenerator>(
            std::move(*generator))};

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
        assert(!reply->generators.empty());
        response.headers().add<Http::Header::Location>(
            "/cpu-generators/" + reply->generators.front()->getId());
        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Created,
            reply->generators[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
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
        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Ok,
            reply->generators[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
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

    auto api_reply =
        submit_request(m_socket.get(), api::request_cpu_generator_del{id});

    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<request_cpu_generator_bulk_add, std::string>
parse_bulk_create_generators(const Rest::Request& request)
{
    auto bulk_request = request_cpu_generator_bulk_add{};
    try {
        const auto j = json::parse(request.body());
        for (const auto& item : j["items"]) {
            bulk_request.generators.emplace_back(
                std::make_unique<cpu_generator_type>(
                    item.get<cpu_generator_type>()));
        }
        return (bulk_request);
    } catch (const json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_create_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto api_request = parse_bulk_create_generators(request);
    if (!api_request) {
        response.send(Http::Code::Bad_Request, api_request.error());
        return;
    }

    /* Verify all ID's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(api_request->generators),
                  std::end(api_request->generators),
                  [&](const auto& gen) {
                      if (!gen->getId().empty()) {
                          if (auto res = config::op_config_validate_id_string(
                                  gen->getId());
                              !res) {
                              id_errors.emplace_back(res.error());
                          }
                      }
                  });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    /* Verify generator objects before forwarding to the server */
    std::vector<std::string> validation_errors;
    std::for_each(std::begin(api_request->generators),
                  std::end(api_request->generators),
                  [&](const auto& gen) { is_valid(*gen, validation_errors); });
    if (!validation_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(validation_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(*api_request));

    if (auto reply = std::get_if<api::reply_cpu_generators>(&api_reply)) {
        auto items = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(items),
                       [](const auto& gen) { return (gen->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, items);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

template <typename T>
tl::expected<T, std::string> parse_request(const Rest::Request& request)
{
    try {
        auto obj = T{};
        auto j = nlohmann::json::parse(request.body());
        obj.fromJson(j);
        return (obj);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_delete_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<BulkDeleteCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    /* Verify ids */
    const auto& ids = swagger_request->getIds();
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (!id.empty()) {
            if (auto res = config::op_config_validate_id_string(id); !res) {
                id_errors.emplace_back(res.error());
            }
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(),
                                    api::request_cpu_generator_bulk_del{
                                        .ids = ids,
                                    });

    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    api::request_cpu_generator_start data{.id = id};

    if (!request.body().empty()) {
        auto dynamic_config = DynamicResultsConfig();
        auto j = json::parse(request.body());
        dynamic_config.fromJson(j);
        auto errors = std::vector<std::string>{};
        if (!is_valid(dynamic_config, errors)) {
            response.send(Http::Code::Bad_Request, concatenate(errors));
            return;
        }
        data.dynamic_results = dynamic::from_swagger(dynamic_config);
    }

    auto api_reply = submit_request(m_socket.get(), std::move(data));

    if (auto reply =
            std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        assert(!reply->results.empty());
        response.headers().add<Http::Header::Location>(
            "/cpu-generator-results/" + reply->results.front()->getId());
        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Created,
            reply->results[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
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
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<BulkStartCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    /* Verify that all user provided id's are valid */
    const auto& ids = swagger_request->getIds();
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (auto res = config::op_config_validate_id_string(id); !res) {
            id_errors.emplace_back(res.error());
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    api::request_cpu_generator_bulk_start data{
        .ids = std::move(swagger_request->getIds())};

    if (swagger_request->dynamicResultsIsSet()) {
        auto errors = std::vector<std::string>{};
        if (!is_valid(*(swagger_request->getDynamicResults()), errors)) {
            response.send(Http::Code::Bad_Request, concatenate(errors));
            return;
        }
        data.dynamic_results =
            dynamic::from_swagger(*(swagger_request->getDynamicResults()));
    }

    auto api_reply = submit_request(m_socket.get(), std::move(data));

    if (auto reply =
            std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        auto items = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(items),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, items);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<BulkStartCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    /* Verify that all user provided id's are valid */
    const auto& ids = swagger_request->getIds();
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (auto res = config::op_config_validate_id_string(id); !res) {
            id_errors.emplace_back(res.error());
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(),
                                    api::request_cpu_generator_bulk_stop{
                                        .ids = ids,
                                    });

    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_generator_results(const Rest::Request&,
                                     Http::ResponseWriter response)
{
    auto api_reply = submit_request(m_socket.get(),
                                    api::request_cpu_generator_result_list{});
    if (auto reply =
            std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        auto items = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(items),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, items);
    } else {
        handle_reply_error(api_reply, std::move(response));
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

    if (auto reply =
            std::get_if<api::reply_cpu_generator_results>(&api_reply)) {
        assert(!reply->results.empty());
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, reply->results[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
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

    auto api_reply = submit_request(m_socket.get(),
                                    api::request_cpu_generator_result_del{id});

    if (std::get_if<api::reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_cpu_info(const Rest::Request&, Http::ResponseWriter response)
{
    auto info = api::get_cpu_info();
    openperf::api::utils::send_chunked_response(
        std::move(response), Http::Code::Ok, info->toJson());
}

} // namespace openperf::cpu::api
