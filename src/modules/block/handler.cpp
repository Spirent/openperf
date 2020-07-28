#include "framework/config/op_config_utils.hpp"
#include "framework/core/op_core.h"
#include "modules/api/api_route_handler.hpp"

#include "api.hpp"
#include "api_converters.hpp"

namespace opneperf::block {

using namespace swagger::v1::model;
using namespace Pistache;
using request_type = Pistache::Rest::Request;
using response_type = Pistache::Http::ResponseWriter;
using json = nlohmann::json;
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

    void init_device(const Rest::Request& request,
                     Http::ResponseWriter response);

    void list_files(const Rest::Request& request,
                    Http::ResponseWriter response);

    void create_file(const Rest::Request& request,
                     Http::ResponseWriter response);

    void get_file(const Rest::Request& request, Http::ResponseWriter response);

    void delete_file(const Rest::Request& request,
                     Http::ResponseWriter response);

    void bulk_create_files(const Rest::Request& request,
                           Http::ResponseWriter response);

    void bulk_delete_files(const Rest::Request& request,
                           Http::ResponseWriter response);

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
};

enum Http::Code to_code(const api::reply_error& error)
{
    switch (error.info.type) {
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
        return ("http://" + host_header->host() + ":"
                + host_header->port().toString());
    }

    return (std::nullopt);
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

handler::handler(void* context, Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, api::endpoint.data()))
{
    Rest::Routes::Get(router,
                      "/block-devices",
                      Rest::Routes::bind(&handler::list_devices, this));
    Rest::Routes::Get(router,
                      "/block-devices/:id",
                      Rest::Routes::bind(&handler::get_device, this));
    Rest::Routes::Post(router,
                       "/block-devices/:id/initialize",
                       Rest::Routes::bind(&handler::init_device, this));
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
    Rest::Routes::Post(router,
                       "/block-files/x/bulk-create",
                       Rest::Routes::bind(&handler::bulk_create_files, this));
    Rest::Routes::Post(router,
                       "/block-files/x/bulk-delete",
                       Rest::Routes::bind(&handler::bulk_delete_files, this));
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
    Rest::Routes::Post(
        router,
        "/block-generators/x/bulk-create",
        Rest::Routes::bind(&handler::bulk_create_generators, this));
    Rest::Routes::Post(
        router,
        "/block-generators/x/bulk-delete",
        Rest::Routes::bind(&handler::bulk_delete_generators, this));
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
    Rest::Routes::Get(
        router,
        "/block-generator-results",
        Rest::Routes::bind(&handler::list_generator_results, this));
    Rest::Routes::Get(router,
                      "/block-generator-results/:id",
                      Rest::Routes::bind(&handler::get_generator_result, this));
    Rest::Routes::Delete(
        router,
        "/block-generator-results/:id",
        Rest::Routes::bind(&handler::delete_generator_result, this));
}

api::reply_msg submit_request(void* socket, api::request_msg&& request)
{
    if (auto error = openperf::message::send(
            socket,
            api::serialize_request(std::forward<api::request_msg>(request)));
        error != 0) {
        return (to_error(api::error_type::ZMQ_ERROR, error));
    }
    auto reply =
        openperf::message::recv(socket).and_then(api::deserialize_reply);
    if (!reply) {
        return (to_error(api::error_type::ZMQ_ERROR, reply.error()));
    }
    return (std::move(*reply));
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
                           return (api::to_swagger(*device)->toJson());
                       });
        response.send(Http::Code::Ok, devices.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
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
        submit_request(m_socket.get(), api::request_block_device{id : id});
    if (auto reply = std::get_if<api::reply_block_devices>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      api::to_swagger(*reply->devices.at(0))->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::init_device(const Rest::Request& request,
                          Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_device_init{id : id});
    if (auto reply = std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
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
                           return (api::to_swagger(*blkfile)->toJson());
                       });
        response.send(Http::Code::Ok, files.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_file(const Rest::Request& request,
                          Http::ResponseWriter response)
{
    try {
        auto file_model = json::parse(request.body()).get<BlockFile>();

        /* Make sure the object is valid before submitting request */
        std::vector<std::string> errors;
        if (!api::is_valid(file_model, errors)) {
            response.send(Http::Code::Bad_Request, concatenate(errors));
            return;
        }

        auto api_reply = submit_request(
            m_socket.get(),
            api::request_block_file_add{
                std::make_unique<api::file_t>(api::from_swagger(file_model))});
        if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
            assert(!reply->files.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            if (auto uri = maybe_get_host_uri(request); uri.has_value()) {
                response.headers().add<Http::Header::Location>(
                    *uri + request.resource() + "/"
                    + reply->files.front()->get_id());
            }
            response.send(
                Http::Code::Created,
                api::to_swagger(*reply->files.front())->toJson().dump());
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
        submit_request(m_socket.get(), api::request_block_file{id : id});
    if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      api::to_swagger(*reply->files.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_file(const Rest::Request& request,
                          Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found);
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_file_del{id : id});
    if (auto reply = std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_create_files(const Rest::Request& request,
                                Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkCreateBlockFilesRequest>();

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (auto reply = std::get_if<api::reply_block_files>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto results = json::array();
        std::transform(std::begin(reply->files),
                       std::end(reply->files),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return (api::to_swagger(*result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_delete_files(const Rest::Request& request,
                                Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkDeleteBlockFilesRequest>();

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
    }
}

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_block_generator_list{});
    if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto generators = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(generators),
                       [](const auto& blkgenerator) {
                           return (api::to_swagger(*blkgenerator)->toJson());
                       });
        response.send(Http::Code::Ok, generators.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    try {
        auto generator_model =
            json::parse(request.body()).get<BlockGenerator>();

        /* Make sure the object is valid before submitting request */
        std::vector<std::string> errors;
        if (!api::is_valid(generator_model, errors)) {
            response.send(Http::Code::Bad_Request, concatenate(errors));
            return;
        }

        auto api_reply = submit_request(
            m_socket.get(),
            api::request_block_generator_add{std::make_unique<api::generator_t>(
                api::from_swagger(generator_model))});
        if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
            assert(!reply->generators.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            if (auto uri = maybe_get_host_uri(request); uri.has_value()) {
                response.headers().add<Http::Header::Location>(
                    *uri + request.resource() + "/"
                    + reply->generators.front()->id());
            }
            response.send(
                Http::Code::Created,
                api::to_swagger(*reply->generators.front())->toJson().dump());
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

    auto api_reply =
        submit_request(m_socket.get(), api::request_block_generator{id : id});
    if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Ok,
            api::to_swagger(*reply->generators.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found);
        return;
    }
    auto api_reply = submit_request(m_socket.get(),
                                    api::request_block_generator_del{id : id});
    if (auto reply = std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_create_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkCreateBlockGeneratorsRequest>();

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (auto reply = std::get_if<api::reply_block_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto results = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return (api::to_swagger(*result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_delete_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkDeleteBlockGeneratorsRequest>();

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

    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_start{id : id});
    if (auto reply =
            std::get_if<api::reply_block_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        if (auto uri = maybe_get_host_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + "/block-generator-results/"
                + reply->results.front()->get_id());
        }
        response.send(
            Http::Code::Created,
            api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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
    }
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkStartBlockGeneratorsRequest>();

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (auto reply =
            std::get_if<api::reply_block_generator_results>(&api_reply)) {
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
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkStopBlockGeneratorsRequest>();

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
    }
}

void handler::list_generator_results(const Rest::Request&,
                                     Http::ResponseWriter response)
{
    auto api_reply = submit_request(m_socket.get(),
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
                           return (api::to_swagger(*result)->toJson());
                       });
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_result{id : id});
    if (auto reply =
            std::get_if<api::reply_block_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Ok,
            api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_generator_result(const Rest::Request& request,
                                      Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found);
        return;
    }
    auto api_reply = submit_request(
        m_socket.get(), api::request_block_generator_result_del{id : id});
    if (auto reply = std::get_if<api::reply_ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

} // namespace opneperf::block
