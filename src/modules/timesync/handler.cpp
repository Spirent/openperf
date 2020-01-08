#include <memory>

#include <json.hpp>
#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "timesync/api.hpp"

namespace opneperf::timesync {

using namespace Pistache;
using json = nlohmann::json;
namespace api = openperf::timesync::api;

class handler : public openperf::api::route::handler::registrar<handler>
{
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    handler(void* context, Rest::Router& router);

    void list_time_counters(const Rest::Request& request,
                            Http::ResponseWriter response);
    void get_time_counter(const Rest::Request& request,
                          Http::ResponseWriter response);

    void get_time_keeper(const Rest::Request& request,
                         Http::ResponseWriter response);

    void list_time_sources(const Rest::Request& request,
                           Http::ResponseWriter response);
    void create_time_source(const Rest::Request& request,
                            Http::ResponseWriter response);
    void get_time_source(const Rest::Request& request,
                         Http::ResponseWriter response);
    void delete_time_source(const Rest::Request& request,
                            Http::ResponseWriter response);
};

enum Http::Code to_code(const api::reply_error& error)
{
    switch (error.info.type) {
    case api::error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case api::error_type::EAI_ERROR:
        return (Http::Code::Unprocessable_Entity);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

const char* to_string(const api::reply_error& error)
{
    switch (error.info.type) {
    case api::error_type::NOT_FOUND:
        return ("");
    case api::error_type::EAI_ERROR:
        return (gai_strerror(error.info.value));
    case api::error_type::ZMQ_ERROR:
        return (zmq_strerror(error.info.value));
    default:
        return ("unknown error type");
    }
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

handler::handler(void* context, Rest::Router& router)
    : m_socket(op_socket_get_client(
        context, ZMQ_REQ, openperf::timesync::api::endpoint.data()))
{
    /* Time counter handlers */
    Rest::Routes::Get(router,
                      "/time-counters",
                      Rest::Routes::bind(&handler::list_time_counters, this));
    Rest::Routes::Get(router,
                      "/time-counters/:id",
                      Rest::Routes::bind(&handler::get_time_counter, this));

    /* Time keeper handlers */
    Rest::Routes::Get(router,
                      "/time-keeper",
                      Rest::Routes::bind(&handler::get_time_keeper, this));

    /* Time source handlers */
    Rest::Routes::Get(router,
                      "/time-sources",
                      Rest::Routes::bind(&handler::list_time_sources, this));
    Rest::Routes::Post(router,
                       "/time-sources",
                       Rest::Routes::bind(&handler::create_time_source, this));
    Rest::Routes::Get(router,
                      "/time-sources/:id",
                      Rest::Routes::bind(&handler::get_time_source, this));
    Rest::Routes::Delete(
        router,
        "/time-sources/:id",
        Rest::Routes::bind(&handler::delete_time_source, this));
}

void handler::list_time_counters(const Rest::Request&,
                                 Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_time_counters{});
    if (auto reply = std::get_if<api::reply_time_counters>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto counters = json::array();
        std::transform(std::begin(reply->counters),
                       std::end(reply->counters),
                       std::back_inserter(counters),
                       [](const auto& counter) {
                           return (to_swagger(counter)->toJson());
                       });
        response.send(Http::Code::Ok, counters.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::get_time_counter(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_time_counters{.id = id});
    if (auto reply = std::get_if<api::reply_time_counters>(&api_reply)) {
        if (reply->counters.empty()) {
            response.send(Http::Code::Not_Found);
        } else {
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(Http::Code::Ok,
                          to_swagger(reply->counters.front())->toJson().dump());
        }
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::get_time_keeper(const Rest::Request&,
                              Http::ResponseWriter response)
{
    auto api_reply = submit_request(m_socket.get(), api::request_time_keeper{});
    if (auto reply = std::get_if<api::reply_time_keeper>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      to_swagger(reply->keeper)->toJson().dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::list_time_sources(const Rest::Request&,
                                Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request_time_sources{});
    if (auto reply = std::get_if<api::reply_time_sources>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto sources = json::array();
        std::transform(
            std::begin(reply->sources),
            std::end(reply->sources),
            std::back_inserter(sources),
            [](const auto& source) { return (to_swagger(source)->toJson()); });
        response.send(Http::Code::Ok, sources.dump());
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_time_source(const Rest::Request& request,
                                 Http::ResponseWriter response)
{
    try {
        auto ts_model =
            json::parse(request.body()).get<swagger::v1::model::TimeSource>();
        auto ts = api::from_swagger(ts_model);
        auto api_reply =
            submit_request(m_socket.get(), api::request_time_source_add{ts});
        if (auto reply = std::get_if<api::reply_time_sources>(&api_reply)) {
            assert(!reply->sources.empty());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(Http::Code::Ok,
                          to_swagger(reply->sources.front())->toJson().dump());
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

void handler::get_time_source(const Rest::Request& request,
                              Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_time_sources{.id = id});
    if (auto reply = std::get_if<api::reply_time_sources>(&api_reply)) {
        if (reply->sources.empty()) {
            response.send(Http::Code::Not_Found);
        } else {
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(Http::Code::Ok,
                          to_swagger(reply->sources.front())->toJson().dump());
        }
    } else if (auto error = std::get_if<api::reply_error>(&api_reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::delete_time_source(const Rest::Request& request,
                                 Http::ResponseWriter response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), api::request_time_source_del{.id = id});
    response.send(Http::Code::No_Content);
}

} // namespace opneperf::timesync
