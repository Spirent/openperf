#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "packet/generator/api.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TogglePacketGeneratorsRequest.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

using namespace Pistache;

/**
 * Various utility functions
 */

static enum Http::Code to_code(const reply_error& error)
{
    switch (error.info.type) {
    case error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case error_type::POSIX:
        return (Http::Code::Bad_Request);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

static const char* to_string(const reply_error& error)
{
    switch (error.info.type) {
    case error_type::NOT_FOUND:
        return ("");
    case error_type::ZMQ_ERROR:
        return (zmq_strerror(error.info.value));
    default:
        return (strerror(error.info.value));
    }
}

static void handle_reply_error(const reply_msg& reply,
                               Http::ResponseWriter response)
{
    if (auto error = std::get_if<reply_error>(&reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

static reply_msg submit_request(void* socket, request_msg&& request)
{
    if (auto error = send_message(
            socket, api::serialize_request(std::forward<request_msg>(request)));
        error != 0) {
        return (to_error(error_type::ZMQ_ERROR, error));
    }

    auto reply = recv_message(socket).and_then(api::deserialize_reply);
    if (!reply) { return (to_error(error_type::ZMQ_ERROR, reply.error())); }

    return (std::move(*reply));
}

static std::string json_error(int code, std::string_view message)
{
    return (
        nlohmann::json({{"code", code}, {"message", message.data()}}).dump());
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

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    /* Generator operations */
    void list_generators(const request_type& request, response_type response);
    void create_generators(const request_type& request, response_type response);
    void delete_generators(const request_type& request, response_type response);
    void get_generator(const request_type& request, response_type response);
    void delete_generator(const request_type& request, response_type response);
    void start_generator(const request_type& request, response_type response);
    void stop_generator(const request_type& request, response_type response);

    /* Bulk generator operations */
    void bulk_create_generators(const request_type& request,
                                response_type response);
    void bulk_delete_generators(const request_type& request,
                                response_type response);
    void bulk_start_generators(const request_type& request,
                               response_type response);
    void bulk_stop_generators(const request_type& request,
                              response_type response);

    /* Replace a running generator with an inactive generator */
    void toggle_generators(const request_type& request, response_type response);

    /* Generator result operations */
    void list_generator_results(const request_type& request,
                                response_type response);
    void delete_generator_results(const request_type& request,
                                  response_type response);
    void get_generator_result(const request_type& request,
                              response_type response);
    void delete_generator_result(const request_type& request,
                                 response_type response);

    /* Tx flow operations */
    void list_tx_flows(const request_type& request, response_type response);
    void get_tx_flow(const request_type& request, response_type response);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

handler::handler(void* context, Pistache::Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/packet/generators", bind(&handler::list_generators, this));
    Post(router, "/packet/generators", bind(&handler::create_generators, this));
    Delete(
        router, "/packet/generators", bind(&handler::delete_generators, this));

    Get(router, "/packet/generators/:id", bind(&handler::get_generator, this));
    Delete(router,
           "/packet/generators/:id",
           bind(&handler::delete_generator, this));

    Post(router,
         "/packet/generators/:id/start",
         bind(&handler::start_generator, this));
    Post(router,
         "/packet/generators/:id/stop",
         bind(&handler::stop_generator, this));

    Post(router,
         "/packet/generators/x/bulk-create",
         bind(&handler::bulk_create_generators, this));
    Post(router,
         "/packet/generators/x/bulk-delete",
         bind(&handler::bulk_delete_generators, this));
    Post(router,
         "/packet/generators/x/bulk-start",
         bind(&handler::bulk_start_generators, this));
    Post(router,
         "/packet/generators/x/bulk-stop",
         bind(&handler::bulk_stop_generators, this));

    Post(router,
         "/packet/generators/x/toggle",
         bind(&handler::toggle_generators, this));

    Get(router,
        "/packet/generator-results",
        bind(&handler::list_generator_results, this));
    Delete(router,
           "/packet/generator-results",
           bind(&handler::delete_generator_results, this));

    Get(router,
        "/packet/generator-results/:id",
        bind(&handler::get_generator_result, this));
    Delete(router,
           "/packet/generator-results/:id",
           bind(&handler::delete_generator_result, this));

    Get(router, "/packet/tx-flows", bind(&handler::list_tx_flows, this));
    Get(router, "/packet/tx-flows/:id", bind(&handler::get_tx_flow, this));
}

static void set_optional_filter(const handler::request_type& request,
                                filter_map_ptr& filter,
                                filter_type key)
{
    if (auto query = request.query().get(std::string(to_filter_name(key)));
        !query.isEmpty()) {
        if (!filter) { filter = std::make_unique<filter_map_type>(); }
        filter->emplace(key, query.get().data());
    }
}

void handler::list_generators(const request_type& request,
                              response_type response)
{
    auto api_request = request_list_generators{};

    set_optional_filter(request, api_request.filter, filter_type::target_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_generators>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto generators = nlohmann::json::array();
        std::transform(
            std::begin(reply->generators),
            std::end(reply->generators),
            std::back_inserter(generators),
            [](const auto& generator) { return (generator->toJson()); });
        response.send(Http::Code::Ok, generators.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static std::optional<std::string>
maybe_get_request_uri(const handler::request_type& request)
{
    if (request.headers().has<Http::Header::Host>()) {
        auto host_header = request.headers().get<Http::Header::Host>();

        /*
         * XXX: Assuming http here. I don't see how to get the type
         * of the connection fomr Pistache.  But for now, that doesn't
         * matter.
         */
        return ("http://" + host_header->host() + ":"
                + host_header->port().toString() + request.resource() + "/");
    }

    return (std::nullopt);
}

template <typename T>
tl::expected<T, std::string> parse_request(const handler::request_type& request)
{
    try {
        auto obj = std::make_shared<T>();
        auto j = nlohmann::json::parse(request.body());
        obj->fromJson(j);
        return (*obj);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

static tl::expected<swagger::v1::model::PacketGenerator, std::string>
parse_create_generator_request(const handler::request_type& request)
{
    try {
        return (nlohmann::json::parse(request.body())
                    .get<swagger::v1::model::PacketGenerator>());
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::create_generators(const request_type& request,
                                response_type response)
{
    auto generator = parse_create_generator_request(request);
    if (!generator) {
        response.send(Http::Code::Bad_Request, generator.error());
        return;
    }

    auto api_request = request_create_generator{
        std::make_unique<swagger::v1::model::PacketGenerator>(
            std::move(*generator))};

    /* If the user provided an id, validate it before forwarding the request */
    if (!api_request.generator->getId().empty()) {
        if (auto res = config::op_config_validate_id_string(
                api_request.generator->getId());
            !res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* Make sure the generator object is valid before submitting request */
    std::vector<std::string> errors;
    if (!is_valid(*api_request.generator, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_generators>(&api_reply)) {
        assert(reply->generators.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->generators[0]->getId());
        }

        response.send(Http::Code::Created,
                      reply->generators[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_generators(const request_type&, response_type response)
{
    auto api_reply =
        submit_request(m_socket.get(), request_delete_generators{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_generator(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_generator{id});

    if (auto reply = std::get_if<reply_generators>(&api_reply)) {
        assert(reply->generators.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->generators[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_generator(const request_type& request,
                               response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_delete_generator{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::start_generator(const request_type& request,
                              response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_start_generator{id});

    if (auto reply = std::get_if<reply_generator_results>(&api_reply)) {
        assert(reply->generator_results.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->generator_results[0]->getId());
        }

        response.send(Http::Code::Created,
                      reply->generator_results[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::stop_generator(const request_type& request,
                             response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_stop_generator{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::bulk_create_generators(const request_type&,
                                     response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_delete_generators(const request_type&,
                                     response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_start_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::bulk_stop_generators(const request_type&, response_type response)
{
    response.send(Http::Code::Not_Implemented);
}

void handler::toggle_generators(const request_type& request,
                                response_type response)
{
    auto toggle =
        parse_request<swagger::v1::model::TogglePacketGeneratorsRequest>(
            request);
    if (!toggle) {
        response.send(Http::Code::Bad_Request, toggle.error());
        return;
    }

    auto ids = std::make_unique<std::pair<std::string, std::string>>(
        toggle->getReplace(), toggle->getWith());

    auto api_reply = submit_request(m_socket.get(),
                                    request_toggle_generator{std::move(ids)});

    if (auto reply = std::get_if<reply_generator_results>(&api_reply)) {
        assert(reply->generator_results.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->generator_results[0]->getId());
        }

        response.send(Http::Code::Created,
                      reply->generator_results[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_generator_results(const request_type& request,
                                     response_type response)
{
    auto api_request = request_list_generator_results{};

    set_optional_filter(request, api_request.filter, filter_type::generator_id);

    set_optional_filter(request, api_request.filter, filter_type::target_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_generator_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto generator_results = nlohmann::json::array();
        std::transform(std::begin(reply->generator_results),
                       std::end(reply->generator_results),
                       std::back_inserter(generator_results),
                       [](const auto& result) { return (result->toJson()); });
        response.send(Http::Code::Ok, generator_results.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_generator_results(const request_type&,
                                       response_type response)
{
    auto api_reply =
        submit_request(m_socket.get(), request_delete_generator_results{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_generator_result(const request_type& request,
                                   response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_get_generator_result{id});

    if (auto reply = std::get_if<reply_generator_results>(&api_reply)) {
        assert(reply->generator_results.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      reply->generator_results[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_generator_result(const request_type& request,
                                      response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_delete_generator_result{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_tx_flows(const request_type& request, response_type response)
{
    auto api_request = request_list_tx_flows{};

    set_optional_filter(request, api_request.filter, filter_type::generator_id);

    set_optional_filter(request, api_request.filter, filter_type::target_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_tx_flows>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto flows = nlohmann::json::array();
        std::transform(std::begin(reply->flows),
                       std::end(reply->flows),
                       std::back_inserter(flows),
                       [](const auto& result) { return (result->toJson()); });
        response.send(Http::Code::Ok, flows.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_tx_flow(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_tx_flow{id});

    if (auto reply = std::get_if<reply_tx_flows>(&api_reply)) {
        assert(reply->flows.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->flows[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

} // namespace openperf::packet::generator::api
