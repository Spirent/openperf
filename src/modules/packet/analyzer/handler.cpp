#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "api/api_utils.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/analyzer/api.hpp"

#include "swagger/converters/packet_analyzer.hpp"
#include "swagger/v1/model/BulkCreatePacketAnalyzersResponse.h"
#include "swagger/v1/model/BulkDeletePacketAnalyzersRequest.h"
#include "swagger/v1/model/BulkStartPacketAnalyzersRequest.h"
#include "swagger/v1/model/BulkStartPacketAnalyzersResponse.h"
#include "swagger/v1/model/BulkStopPacketAnalyzersRequest.h"
#include "swagger/v1/model/PacketAnalyzer.h"
#include "swagger/v1/model/PacketAnalyzerResult.h"
#include "swagger/v1/model/RxFlow.h"

namespace openperf::packet::analyzer::api {

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    /* PacketAnalyzer operations */
    void list_analyzers(const request_type& request, response_type response);
    void create_analyzers(const request_type& request, response_type response);
    void delete_analyzers(const request_type& request, response_type response);
    void get_analyzer(const request_type& request, response_type response);
    void delete_analyzer(const request_type& request, response_type response);
    void reset_analyzer(const request_type& request, response_type response);
    void start_analyzer(const request_type& request, response_type response);
    void stop_analyzer(const request_type& request, response_type response);

    /* Bulk analyzer operations */
    void bulk_create_analyzers(const request_type& request,
                               response_type response);
    void bulk_delete_analyzers(const request_type& request,
                               response_type response);
    void bulk_start_analyzers(const request_type& request,
                              response_type response);
    void bulk_stop_analyzers(const request_type& request,
                             response_type response);

    /* PacketAnalyzerResult operations */
    void list_analyzer_results(const request_type& request,
                               response_type response);
    void delete_analyzer_results(const request_type& request,
                                 response_type response);
    void get_analyzer_result(const request_type& request,
                             response_type response);
    void delete_analyzer_result(const request_type& request,
                                response_type response);

    /* Rx flow operations */
    void list_rx_flows(const request_type& request, response_type response);
    void get_rx_flow(const request_type& request, response_type response);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

handler::handler(void* context, Pistache::Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/packet/analyzers", bind(&handler::list_analyzers, this));
    Post(router, "/packet/analyzers", bind(&handler::create_analyzers, this));
    Delete(router, "/packet/analyzers", bind(&handler::delete_analyzers, this));

    Get(router, "/packet/analyzers/:id", bind(&handler::get_analyzer, this));
    Delete(
        router, "/packet/analyzers/:id", bind(&handler::delete_analyzer, this));

    Post(router,
         "/packet/analyzers/:id/reset",
         bind(&handler::reset_analyzer, this));
    Post(router,
         "/packet/analyzers/:id/start",
         bind(&handler::start_analyzer, this));
    Post(router,
         "/packet/analyzers/:id/stop",
         bind(&handler::stop_analyzer, this));

    Post(router,
         "/packet/analyzers/x/bulk-create",
         bind(&handler::bulk_create_analyzers, this));
    Post(router,
         "/packet/analyzers/x/bulk-delete",
         bind(&handler::bulk_delete_analyzers, this));
    Post(router,
         "/packet/analyzers/x/bulk-start",
         bind(&handler::bulk_start_analyzers, this));
    Post(router,
         "/packet/analyzers/x/bulk-stop",
         bind(&handler::bulk_stop_analyzers, this));

    Get(router,
        "/packet/analyzer-results",
        bind(&handler::list_analyzer_results, this));
    Delete(router,
           "/packet/analyzer-results",
           bind(&handler::delete_analyzer_results, this));

    Get(router,
        "/packet/analyzer-results/:id",
        bind(&handler::get_analyzer_result, this));
    Delete(router,
           "/packet/analyzer-results/:id",
           bind(&handler::delete_analyzer_result, this));

    Get(router, "/packet/rx-flows", bind(&handler::list_rx_flows, this));
    Get(router, "/packet/rx-flows/:id", bind(&handler::get_rx_flow, this));
}

using namespace Pistache;

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

static const char* to_string(const api::reply_error& error)
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
                               Pistache::Http::ResponseWriter response)
{
    if (auto error = std::get_if<reply_error>(&reply)) {
        response.send(to_code(*error), to_string(*error));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

static reply_msg submit_request(void* socket, request_msg&& request)
{
    if (auto error = message::send(
            socket, api::serialize_request(std::forward<request_msg>(request)));
        error != 0) {
        return (to_error(error_type::ZMQ_ERROR, error));
    }

    auto reply = message::recv(socket).and_then(api::deserialize_reply);
    if (!reply) { return (to_error(error_type::ZMQ_ERROR, reply.error())); }

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

void set_optional_filter(const handler::request_type& request,
                         filter_map_ptr& filter,
                         filter_key_type key)
{
    if (auto query = request.query().get(std::string(to_key_name(key)));
        !query.isEmpty()) {
        if (!filter) { filter = std::make_unique<filter_map_type>(); }
        filter->emplace(key, query.get().data());
    }
}

void handler::list_analyzers(const request_type& request,
                             response_type response)
{
    auto api_request = request_list_analyzers{};

    set_optional_filter(
        request, api_request.filter, filter_key_type::source_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_analyzers>(&api_reply)) {
        auto analyzers = nlohmann::json::array();
        std::transform(
            std::begin(reply->analyzers),
            std::end(reply->analyzers),
            std::back_inserter(analyzers),
            [](const auto& analyzer) { return (analyzer->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, analyzers);
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
         * XXX: Assuming http here.  I don't know how to get the type
         * of the connection from Pistache...  But for now, that doesn't
         * matter.
         */
        return ("http://" + host_header->host() + ":"
                + host_header->port().toString() + request.resource() + "/");
    }

    return (std::nullopt);
}

static tl::expected<swagger::v1::model::PacketAnalyzer, std::string>
parse_create_analyzer(const handler::request_type& request)
{
    try {
        return (nlohmann::json::parse(request.body())
                    .get<swagger::v1::model::PacketAnalyzer>());
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::create_analyzers(const request_type& request,
                               response_type response)
{
    auto analyzer = parse_create_analyzer(request);
    if (!analyzer) {
        response.send(Http::Code::Bad_Request, analyzer.error());
        return;
    }

    auto api_request = request_create_analyzer{
        std::make_unique<swagger::v1::model::PacketAnalyzer>(
            std::move(*analyzer))};

    /* If the user provided an id, validate it before forwarding the request */
    if (!api_request.analyzer->getId().empty()) {
        auto res =
            config::op_config_validate_id_string(api_request.analyzer->getId());
        if (!res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* Make sure the analyzer object is valid before forwarding to the server */
    std::vector<std::string> errors;
    if (!is_valid(*api_request.analyzer, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_analyzers>(&api_reply)) {
        assert(reply->analyzers.size() == 1);

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->analyzers[0]->getId());
        }

        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Created,
            reply->analyzers[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_analyzers(const request_type&, response_type response)
{
    auto api_reply = submit_request(m_socket.get(), request_delete_analyzers{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_analyzer(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_analyzer{id});

    if (auto reply = std::get_if<reply_analyzers>(&api_reply)) {
        assert(reply->analyzers.size() == 1);
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, reply->analyzers[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_analyzer(const request_type& request,
                              response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_delete_analyzer{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::reset_analyzer(const request_type& request,
                             response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_reset_analyzer{id});

    if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
        assert(reply->analyzer_results.size() == 1);

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->analyzer_results[0]->getId());
        }

        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Created,
            reply->analyzer_results[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::start_analyzer(const request_type& request,
                             response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_start_analyzer{id});

    if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
        assert(reply->analyzer_results.size() == 1);

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->analyzer_results[0]->getId());
        }

        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Created,
            reply->analyzer_results[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::stop_analyzer(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_stop_analyzer{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<request_bulk_create_analyzers, std::string>
parse_bulk_create_analyzers(const handler::request_type& request)
{
    auto bulk_request = request_bulk_create_analyzers{};
    try {
        const auto j = nlohmann::json::parse(request.body());
        for (auto&& item : j["items"]) {
            bulk_request.analyzers.emplace_back(
                std::make_unique<analyzer_type>(item.get<analyzer_type>()));
        }
        return (bulk_request);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_create_analyzers(const request_type& request,
                                    response_type response)
{
    auto api_request = parse_bulk_create_analyzers(request);
    if (!api_request) {
        response.send(Http::Code::Bad_Request, api_request.error());
        return;
    }

    /* Verify that all user provided id's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(api_request->analyzers),
                  std::end(api_request->analyzers),
                  [&](const auto& analyzer) {
                      if (!analyzer->getId().empty()) {
                          if (auto res = config::op_config_validate_id_string(
                                  analyzer->getId());
                              !res) {
                              id_errors.emplace_back(res.error());
                          }
                      }
                  });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    /* Validate all analzyer objects before forwarding to the server */
    std::vector<std::string> validation_errors;
    std::for_each(
        std::begin(api_request->analyzers),
        std::end(api_request->analyzers),
        [&](const auto& analyzer) { is_valid(*analyzer, validation_errors); });
    if (!validation_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(validation_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(*api_request));

    if (auto reply = std::get_if<reply_analyzers>(&api_reply)) {
        auto swagger_reply =
            swagger::v1::model::BulkCreatePacketAnalyzersResponse{};
        std::move(std::begin(reply->analyzers),
                  std::end(reply->analyzers),
                  std::back_inserter(swagger_reply.getItems()));

        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, swagger_reply.toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

template <typename T>
tl::expected<T, std::string> parse_request(const handler::request_type& request)
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

void handler::bulk_delete_analyzers(const request_type& request,
                                    response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkDeletePacketAnalyzersRequest>(
            request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    const auto& ids = swagger_request->getIds();

    /* Verify that all user provided id's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (!id.empty()) {
            if (auto res = config::op_config_validate_id_string(id); !res) {
                id_errors.emplace_back(res.error());
            }
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    auto api_request = request_bulk_delete_analyzers{};

    std::transform(
        std::begin(ids),
        std::end(ids),
        std::back_inserter(api_request.ids),
        [](const auto& id) { return (std::make_unique<std::string>(id)); });

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::bulk_start_analyzers(const request_type& request,
                                   response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkStartPacketAnalyzersRequest>(
            request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
        return;
    }

    const auto& ids = swagger_request->getIds();

    /* Verify that all user provided id's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (!id.empty()) {
            if (auto res = config::op_config_validate_id_string(id); !res) {
                id_errors.emplace_back(res.error());
            }
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    auto api_request = request_bulk_start_analyzers{};

    std::transform(
        std::begin(ids),
        std::end(ids),
        std::back_inserter(api_request.ids),
        [](const auto& id) { return (std::make_unique<std::string>(id)); });

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
        auto swagger_reply =
            swagger::v1::model::BulkStartPacketAnalyzersResponse{};
        std::move(std::begin(reply->analyzer_results),
                  std::end(reply->analyzer_results),
                  std::back_inserter(swagger_reply.getItems()));

        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, swagger_reply.toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::bulk_stop_analyzers(const request_type& request,
                                  response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkStopPacketAnalyzersRequest>(
            request);
    if (!swagger_request) {
        response.send(Http::Code::Bad_Request, swagger_request.error());
    }

    const auto& ids = swagger_request->getIds();

    /* Verify that all user provided id's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (!id.empty()) {
            if (auto res = config::op_config_validate_id_string(id); !res) {
                id_errors.emplace_back(res.error());
            }
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    auto api_request = request_bulk_stop_analyzers{};

    std::transform(
        std::begin(ids),
        std::end(ids),
        std::back_inserter(api_request.ids),
        [](const auto& id) { return (std::make_unique<std::string>(id)); });

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_analyzer_results(const request_type& request,
                                    response_type response)
{
    auto api_request = request_list_analyzer_results{};

    set_optional_filter(
        request, api_request.filter, filter_key_type::analyzer_id);

    set_optional_filter(
        request, api_request.filter, filter_key_type::source_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
        auto analyzer_results = nlohmann::json::array();
        std::transform(std::begin(reply->analyzer_results),
                       std::end(reply->analyzer_results),
                       std::back_inserter(analyzer_results),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, analyzer_results);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_analyzer_results(const request_type&,
                                      response_type response)
{
    auto api_reply =
        submit_request(m_socket.get(), request_delete_analyzer_results{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_analyzer_result(const request_type& request,
                                  response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_get_analyzer_result{id});

    if (auto reply = std::get_if<reply_analyzer_results>(&api_reply)) {
        assert(reply->analyzer_results.size() == 1);
        openperf::api::utils::send_chunked_response(
            std::move(response),
            Http::Code::Ok,
            reply->analyzer_results[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_analyzer_result(const request_type& request,
                                     response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_delete_analyzer_result{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_rx_flows(const request_type& request, response_type response)
{
    auto api_request = request_list_rx_flows{};

    set_optional_filter(
        request, api_request.filter, filter_key_type::analyzer_id);

    set_optional_filter(
        request, api_request.filter, filter_key_type::source_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_rx_flows>(&api_reply)) {
        auto flows = nlohmann::json::array();
        std::transform(std::begin(reply->flows),
                       std::end(reply->flows),
                       std::back_inserter(flows),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, flows);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_rx_flow(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_rx_flow{id});

    if (auto reply = std::get_if<reply_rx_flows>(&api_reply)) {
        assert(reply->flows.size() == 1);
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, reply->flows[0]->toJson());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

} // namespace openperf::packet::analyzer::api
