#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packetio/port_api.hpp"

#include "swagger/converters/packetio.hpp"
#include "swagger/v1/model/Port.h"

namespace openperf::packetio::port::api {

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    void list_ports(const request_type& request, response_type response);
    void create_port(const request_type& request, response_type response);
    void get_port(const request_type& request, response_type response);
    void update_port(const request_type& request, response_type response);
    void delete_port(const request_type& request, response_type response);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

handler::handler(void* context, Pistache::Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/ports", bind(&handler::list_ports, this));
    Post(router, "/ports", bind(&handler::create_port, this));
    Get(router, "/ports/:id", bind(&handler::get_port, this));
    Put(router, "/ports/:id", bind(&handler::update_port, this));
    Delete(router, "/ports/:id", bind(&handler::delete_port, this));
}

using namespace Pistache;

static enum Http::Code to_code(const reply_error& error)
{
    switch (error.type) {
    case error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case error_type::VERBOSE:
        return (Http::Code::Bad_Request);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

static reply_error to_error(error_type type, std::string_view msg)
{
    return (reply_error{.type = type, .msg = std::string(msg)});
};

static void handle_reply_error(const reply_msg& reply,
                               handler::response_type response)
{
    if (auto error = std::get_if<reply_error>(&reply)) {
        response.send(to_code(*error), error->msg);
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

static reply_msg submit_request(void* socket, request_msg&& request)
{
    if (auto error = message::send(
            socket, api::serialize_request(std::forward<request_msg>(request)));
        error != 0) {
        return (to_error(error_type::VERBOSE, zmq_strerror(error)));
    }

    auto reply = message::recv(socket).and_then(api::deserialize_reply);
    if (!reply) {
        return (to_error(error_type::VERBOSE, zmq_strerror(reply.error())));
    }

    return (std::move(*reply));
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

static void set_optional_filter(const handler::request_type& request,
                                filter_map_ptr& filter,
                                filter_key_type key)
{
    if (auto query = request.query().get(std::string(to_key_name(key)));
        !query.isEmpty()) {
        if (!filter) { filter = std::make_unique<filter_map_type>(); }
        filter->emplace(key, query.get().data());
    }
}

void handler::list_ports(const request_type& request, response_type response)
{
    auto api_request = request_list_ports{};

    set_optional_filter(request, api_request.filter, filter_key_type::kind);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ports>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto ports = nlohmann::json::array();
        std::transform(std::begin(reply->ports),
                       std::end(reply->ports),
                       std::back_inserter(ports),
                       [](const auto& port) { return (port->toJson()); });
        response.send(Http::Code::Ok, ports.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<swagger::v1::model::Port, std::string>
parse_create_port(const handler::request_type& request)
{
    try {
        return (nlohmann::json::parse(request.body())
                    .get<swagger::v1::model::Port>());
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

static std::optional<std::string>
maybe_get_request_uri(const handler::request_type& request)
{
    if (request.headers().has<Http::Header::Host>()) {
        auto host_header = request.headers().get<Http::Header::Host>();
        return (request.resource());
    }

    return (std::nullopt);
}

void handler::create_port(const request_type& request, response_type response)
{
    auto port = parse_create_port(request);
    if (!port) {
        response.send(Http::Code::Bad_Request, port.error());
        return;
    }

    auto api_request = request_create_port{
        std::make_unique<swagger::v1::model::Port>(std::move(*port))};

    /* Ensure we have a valid id */
    if (!api_request.port->getId().empty()) {
        auto res =
            config::op_config_validate_id_string(api_request.port->getId());
        if (!res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* And a valid configuration */
    std::vector<std::string> errors;
    if (!is_valid(*api_request.port, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ports>(&api_reply)) {
        assert(reply->ports.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + "/" + reply->ports[0]->getId());
        }

        response.send(Http::Code::Created, reply->ports[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_port(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_port{id});

    if (auto reply = std::get_if<reply_ports>(&api_reply)) {
        assert(reply->ports.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->ports[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::update_port(const request_type& request, response_type response)
{
    auto port = parse_create_port(request);
    if (!port) {
        response.send(Http::Code::Bad_Request, port.error());
        return;
    }

    auto api_request = request_update_port{
        std::make_unique<swagger::v1::model::Port>(std::move(*port))};

    /* Ensure we have a valid id */
    if (!api_request.port->getId().empty()) {
        auto res =
            config::op_config_validate_id_string(api_request.port->getId());
        if (!res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* And a valid configuration */
    std::vector<std::string> errors;
    if (!is_valid(*api_request.port, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ports>(&api_reply)) {
        assert(reply->ports.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->ports[0]->getId());
        }

        response.send(Http::Code::Created, reply->ports[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_port(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_delete_port{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

} // namespace openperf::packetio::port::api
