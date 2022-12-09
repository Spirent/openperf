#include <zmq.h>

#include "api/api_rest_handler.tcc"
#include "api/api_route_handler.hpp"
#include "api/api_utils.hpp"
#include "cpu/api.hpp"
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
namespace model = swagger::v1::model;
using json = nlohmann::json;

static api::reply_msg submit_request(void* socket, api::request_msg&& request)
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

struct rest_traits
{
    using reply_type = api::reply_msg;
    using request_type = api::request_msg;

    using request_object_list_type = api::request_cpu_generator_list;
    using request_object_get_type = api::request_cpu_generator;
    using request_object_create_type = api::request_cpu_generator_add;
    using request_object_delete_type = api::request_cpu_generator_del;
    using request_object_start_type = api::request_cpu_generator_start;
    using request_object_stop_type = api::request_cpu_generator_stop;

    using request_result_list_type = api::request_cpu_generator_result_list;
    using request_result_get_type = api::request_cpu_generator_result;
    using request_result_delete_type = api::request_cpu_generator_result_del;

    using request_object_bulk_create_type = api::request_cpu_generator_bulk_add;
    using request_object_bulk_delete_type = api::request_cpu_generator_bulk_del;
    using request_object_bulk_start_type =
        api::request_cpu_generator_bulk_start;
    using request_object_bulk_stop_type = api::request_cpu_generator_bulk_stop;

    using reply_object_container_type = api::reply_cpu_generators;
    using reply_result_container_type = api::reply_cpu_generator_results;
    using reply_ok_type = api::reply_ok;

    struct object_validator_type
    {
        bool operator()(const model::CpuGenerator& generator,
                        std::vector<std::string>& errors)
        {
            return (is_valid(generator, errors));
        }
    };

    struct object_extractor_type
    {
        const api::cpu_generator_ptr&
        operator()(const request_object_create_type& wrapper)
        {
            return (wrapper.generator);
        }

        const std::vector<api::cpu_generator_ptr>&
        operator()(const request_object_bulk_create_type& container)
        {
            return (container.generators);
        }

        const std::vector<api::cpu_generator_ptr>&
        operator()(const reply_object_container_type& container)
        {
            return (container.generators);
        }
    };

    struct dynamic_result_validator_type
    {
        bool operator()(const model::DynamicResultsConfig& config,
                        std::vector<std::string>& errors)
        {
            return (is_valid(config, errors));
        }
    };

    struct result_extractor_type
    {
        const std::vector<api::cpu_generator_result_ptr>&
        operator()(const reply_result_container_type& container)
        {
            return (container.results);
        }
    };

    static constexpr auto object_path_prefix = "/cpu-generators/";
    static constexpr auto result_path_prefix = "/cpu-generator-results/";
};

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

    void list_generator_results(const Rest::Request& request,
                                Http::ResponseWriter response);

    void get_generator_result(const Rest::Request& request,
                              Http::ResponseWriter response);

    void delete_generator_result(const Rest::Request& request,
                                 Http::ResponseWriter response);

    void get_cpu_info(const Rest::Request& request,
                      Http::ResponseWriter response);

    void bulk_create_generators(const Rest::Request& request,
                                Http::ResponseWriter response);

    void bulk_delete_generators(const Rest::Request& request,
                                Http::ResponseWriter response);

    void bulk_start_generators(const Rest::Request& request,
                               Http::ResponseWriter response);

    void bulk_stop_generators(const Rest::Request& request,
                              Http::ResponseWriter response);
};

static void handle_reply_error(const api::reply_msg& reply,
                               Pistache::Http::ResponseWriter response)
{
    if (auto error = std::get_if<api::reply_error>(&reply)) {
        auto [code, string] = openperf::api::rest::decode_error(error->info);
        response.send(code, string.value_or(""));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-create",
        Rest::Routes::bind(&handler::bulk_create_generators, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-delete",
        Rest::Routes::bind(&handler::bulk_delete_generators, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-start",
        Rest::Routes::bind(&handler::bulk_start_generators, this));
    Rest::Routes::Post(
        router,
        "/cpu-generators/x/bulk-stop",
        Rest::Routes::bind(&handler::bulk_stop_generators, this));
}

void handler::list_generators(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    openperf::api::rest::do_list_objects<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

static tl::expected<swagger::v1::model::CpuGenerator, std::string>
parse_create_generator(const Pistache::Rest::Request& request)
{
    try {
        return (json::parse(request.body())
                    .get<swagger::v1::model::CpuGenerator>());
    } catch (const json::exception& e) {
        return (tl::unexpected(
            openperf::api::rest::to_json(e.id, e.what()).dump()));
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

    openperf::api::rest::do_create_object<rest_traits>(
        std::make_unique<swagger::v1::model::CpuGenerator>(
            std::move(*generator)),
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::get_generator(const Rest::Request& request,
                            Http::ResponseWriter response)
{
    openperf::api::rest::do_get_object<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    openperf::api::rest::do_delete_object<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::start_generator(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    openperf::api::rest::do_start_object<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::stop_generator(const Rest::Request& request,
                             Http::ResponseWriter response)
{
    openperf::api::rest::do_stop_object<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::list_generator_results(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    openperf::api::rest::do_list_results<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::get_generator_result(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    openperf::api::rest::do_get_result<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::delete_generator_result(const Rest::Request& request,
                                      Http::ResponseWriter response)
{
    openperf::api::rest::do_delete_result<rest_traits>(
        request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::get_cpu_info(const Rest::Request&, Http::ResponseWriter response)
{
    auto info = api::get_cpu_info();
    openperf::api::utils::send_chunked_response(
        std::move(response), Http::Code::Ok, info->toJson());
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
        return (tl::unexpected(
            openperf::api::rest::to_json(e.id, e.what()).dump()));
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

    openperf::api::rest::do_bulk_create_objects<rest_traits>(
        std::move(*api_request),
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
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
        return (tl::unexpected(
            openperf::api::rest::to_json(e.id, e.what()).dump()));
    }
}

void handler::bulk_delete_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<model::BulkDeleteCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    openperf::api::rest::do_bulk_delete_objects<rest_traits>(
        *swagger_request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<model::BulkStartCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    openperf::api::rest::do_bulk_start_objects<rest_traits>(
        *swagger_request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto swagger_request =
        parse_request<model::BulkStopCpuGeneratorsRequest>(request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    openperf::api::rest::do_bulk_stop_objects<rest_traits>(
        *swagger_request,
        std::move(response),
        [&](api::request_msg&& request) {
            return (submit_request(m_socket.get(),
                                   std::forward<api::request_msg>(request)));
        },
        handle_reply_error);
}

} // namespace openperf::cpu::api
