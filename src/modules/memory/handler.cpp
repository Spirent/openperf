#include "zmq.h"

#include "api/api_rest_handler.tcc"
#include "api/api_route_handler.hpp"
#include "dynamic/api.hpp"
#include "memory/api.hpp"
#include "framework/config/op_config_utils.hpp"
#include "framework/core/op_core.h"
#include "framework/message/serialized_message.hpp"

#include "swagger/converters/memory.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"
#include "swagger/v1/model/MemoryInfoResult.h"
#include "swagger/v1/model/BulkDeleteMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartMemoryGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopMemoryGeneratorsRequest.h"

namespace openperf::memory::api {

namespace model = ::swagger::v1::model;
using json = nlohmann::json;
using namespace Pistache;

static api::reply_msg submit_request(void* socket, api::request_msg&& request)
{
    if (auto error = openperf::message::send(
            socket, api::serialize(std::forward<api::request_msg>(request)));
        error != 0) {
        return reply::error{.info = {
                                .type = reply::error_type::ZMQ_ERROR,
                                .value = error,
                            }};
    }

    auto reply =
        openperf::message::recv(socket).and_then(api::deserialize_reply);
    if (!reply) {
        return reply::error{.info = {
                                .type = reply::error_type::ZMQ_ERROR,
                                .value = reply.error(),
                            }};
    }

    return std::move(*reply);
}

struct rest_traits
{
    using reply_type = reply_msg;
    using request_type = request_msg;

    using request_object_list_type = request::generator::list;
    using request_object_get_type = request::generator::get;
    using request_object_create_type = request::generator::create;
    using request_object_delete_type = request::generator::erase;
    using request_object_start_type = request::generator::start;
    using request_object_stop_type = request::generator::stop;

    using request_result_list_type = request::result::list;
    using request_result_get_type = request::result::get;
    using request_result_delete_type = request::result::erase;

    using request_object_bulk_create_type = request::generator::bulk::create;
    ;
    using request_object_bulk_delete_type = request::generator::bulk::erase;
    using request_object_bulk_start_type = request::generator::bulk::start;
    using request_object_bulk_stop_type = request::generator::bulk::stop;

    using reply_object_container_type = reply::generators;
    using reply_result_container_type = reply::results;
    using reply_ok_type = reply::ok;

    struct object_validator_type
    {
        bool operator()(const model::MemoryGenerator& generator,
                        std::vector<std::string>& errors)
        {
            return (is_valid(generator, errors));
        }
    };

    struct object_extractor_type
    {
        const api::mem_generator_ptr&
        operator()(const request_object_create_type& wrapper)
        {
            return (wrapper.generator);
        }

        const std::vector<api::mem_generator_ptr>&
        operator()(const request_object_bulk_create_type& container)
        {
            return (container.generators);
        }

        const std::vector<api::mem_generator_ptr>&
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
        const std::vector<api::mem_generator_result_ptr>&
        operator()(const reply_result_container_type& container)
        {
            return (container.results);
        }
    };

    static constexpr auto object_path_prefix = "/memory-generators/";
    static constexpr auto result_path_prefix = "/memory-generator-results/";
};

void handle_reply_error(const api::reply_msg& reply,
                        Http::ResponseWriter response)
{
    if (auto* error = std::get_if<api::reply::error>(&reply)) {
        auto [code, string] = openperf::api::rest::decode_error(error->info);
        response.send(code, string.value_or(""));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

class handler : public openperf::api::route::handler::registrar<handler>
{
private:
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    handler(void* context, Rest::Router&);

    // Memory generator actions
    void list_generators(const Rest::Request&, Http::ResponseWriter);
    void create_generator(const Rest::Request&, Http::ResponseWriter);
    void get_generator(const Rest::Request&, Http::ResponseWriter);
    void delete_generator(const Rest::Request&, Http::ResponseWriter);
    void start_generator(const Rest::Request&, Http::ResponseWriter);
    void stop_generator(const Rest::Request&, Http::ResponseWriter);

    // Memory generator statistic actions
    void list_results(const Rest::Request&, Http::ResponseWriter);
    void get_result(const Rest::Request&, Http::ResponseWriter);
    void delete_result(const Rest::Request&, Http::ResponseWriter);

    // Memory information
    void get_mem_info(const Rest::Request&, Http::ResponseWriter);

    // Bulk memory generator actions
    void bulk_create_generators(const Rest::Request&, Http::ResponseWriter);
    void bulk_delete_generators(const Rest::Request&, Http::ResponseWriter);
    void bulk_start_generators(const Rest::Request&, Http::ResponseWriter);
    void bulk_stop_generators(const Rest::Request&, Http::ResponseWriter);
};

handler::handler(void* context, Rest::Router& router)
    : m_socket(
        op_socket_get_client(context, ZMQ_REQ, openperf::memory::api::endpoint))
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

    Rest::Routes::Get(router,
                      "/memory-generator-results",
                      Rest::Routes::bind(&handler::list_results, this));
    Rest::Routes::Get(router,
                      "/memory-generator-results/:id",
                      Rest::Routes::bind(&handler::get_result, this));
    Rest::Routes::Delete(router,
                         "/memory-generator-results/:id",
                         Rest::Routes::bind(&handler::delete_result, this));

    Rest::Routes::Get(router,
                      "/memory-info",
                      Rest::Routes::bind(&handler::get_mem_info, this));

    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-create",
        Rest::Routes::bind(&handler::bulk_create_generators, this));
    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-delete",
        Rest::Routes::bind(&handler::bulk_delete_generators, this));
    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-start",
        Rest::Routes::bind(&handler::bulk_start_generators, this));
    Rest::Routes::Post(
        router,
        "/memory-generators/x/bulk-stop",
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

static tl::expected<swagger::v1::model::MemoryGenerator, std::string>
parse_create_generator(const Rest::Request& request)
{
    try {
        return (json::parse(request.body()).get<model::MemoryGenerator>());
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
        std::make_unique<model::MemoryGenerator>(std::move(*generator)),
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

// Memory generator results
void handler::list_results(const Rest::Request& request,
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

void handler::get_result(const Rest::Request& request,
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

void handler::delete_result(const Rest::Request& request,
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

void handler::get_mem_info(const Rest::Request&, Http::ResponseWriter response)
{
    auto info = get_memory_info();
    openperf::api::utils::send_chunked_response(
        std::move(response), Http::Code::Ok, info->toJson());
}

// Bulk memory generator actions
static tl::expected<request::generator::bulk::create, std::string>
parse_bulk_create_generators(const Rest::Request& request)
{
    auto bulk_request = request::generator::bulk::create{};
    try {
        const auto j = json::parse(request.body());
        for (const auto& item : j["items"]) {
            bulk_request.generators.emplace_back(
                std::make_unique<mem_generator_type>(
                    item.get<mem_generator_type>()));
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
        parse_request<model::BulkDeleteMemoryGeneratorsRequest>(request);
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
        parse_request<model::BulkStartMemoryGeneratorsRequest>(request);
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
        parse_request<model::BulkStopMemoryGeneratorsRequest>(request);
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

} // namespace openperf::memory::api
