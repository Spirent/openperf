#include <zmq.h>
#include <sys/stat.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/capture/api.hpp"
#include "packet/capture/pcap_transfer.hpp"

#include "swagger/v1/model/BulkCreatePacketCapturesResponse.h"
#include "swagger/v1/model/BulkDeletePacketCapturesRequest.h"
#include "swagger/v1/model/BulkStartPacketCapturesRequest.h"
#include "swagger/v1/model/BulkStartPacketCapturesResponse.h"
#include "swagger/v1/model/BulkStopPacketCapturesRequest.h"
#include "swagger/v1/model/PacketCapture.h"
#include "swagger/v1/model/PacketCaptureResult.h"

// using namespace openperf::packet::capture;

namespace openperf::packet::capture::api {

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    /* Capture operations */
    void list_captures(const request_type& request, response_type response);
    void create_captures(const request_type& request, response_type response);
    void delete_captures(const request_type& request, response_type response);
    void get_capture(const request_type& request, response_type response);
    void delete_capture(const request_type& request, response_type response);
    void start_capture(const request_type& request, response_type response);
    void stop_capture(const request_type& request, response_type response);

    /* Bulk capture operations */
    void bulk_create_captures(const request_type& request,
                              response_type response);
    void bulk_delete_captures(const request_type& request,
                              response_type response);
    void bulk_start_captures(const request_type& request,
                             response_type response);
    void bulk_stop_captures(const request_type& request,
                            response_type response);

    /* Capture result operations */
    void list_capture_results(const request_type& request,
                              response_type response);
    void delete_capture_results(const request_type& request,
                                response_type response);
    void get_capture_result(const request_type& request,
                            response_type response);
    void delete_capture_result(const request_type& request,
                               response_type response);
    void get_capture_result_pcap(const request_type& request,
                                 response_type response);
    void get_capture_result_live(const request_type& request,
                                 response_type response);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

handler::handler(void* context, Pistache::Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/packet/captures", bind(&handler::list_captures, this));
    Post(router, "/packet/captures", bind(&handler::create_captures, this));
    Delete(router, "/packet/captures", bind(&handler::delete_captures, this));

    Get(router, "/packet/captures/:id", bind(&handler::get_capture, this));
    Delete(
        router, "/packet/captures/:id", bind(&handler::delete_capture, this));

    Post(router,
         "/packet/captures/:id/start",
         bind(&handler::start_capture, this));
    Post(router,
         "/packet/captures/:id/stop",
         bind(&handler::stop_capture, this));

    Post(router,
         "/packet/captures/x/bulk-create",
         bind(&handler::bulk_create_captures, this));
    Post(router,
         "/packet/captures/x/bulk-delete",
         bind(&handler::bulk_delete_captures, this));
    Post(router,
         "/packet/captures/x/bulk-start",
         bind(&handler::bulk_start_captures, this));
    Post(router,
         "/packet/captures/x/bulk-stop",
         bind(&handler::bulk_stop_captures, this));

    Get(router,
        "/packet/capture-results",
        bind(&handler::list_capture_results, this));
    Delete(router,
           "/packet/capture-results",
           bind(&handler::delete_capture_results, this));

    Get(router,
        "/packet/capture-results/:id",
        bind(&handler::get_capture_result, this));
    Delete(router,
           "/packet/capture-results/:id",
           bind(&handler::delete_capture_result, this));

    Get(router,
        "/packet/capture-results/:id/pcap",
        bind(&handler::get_capture_result_pcap, this));
    Get(router,
        "/packet/capture-results/:id/live",
        bind(&handler::get_capture_result_live, this));
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

static tl::expected<uint64_t, int> to_uint64(const std::string& val_str)
{
    char* end_ptr = nullptr;
    auto v = strtoull(val_str.c_str(), &end_ptr, 10);
    if (!end_ptr || *end_ptr != '\0') { return tl::make_unexpected(EINVAL); }
    return v;
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

void handler::list_captures(const request_type& request, response_type response)
{
    auto api_request = request_list_captures{};

    set_optional_filter(
        request, api_request.filter, filter_key_type::source_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_captures>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto captures = nlohmann::json::array();
        std::transform(std::begin(reply->captures),
                       std::end(reply->captures),
                       std::back_inserter(captures),
                       [](const auto& capture) { return (capture->toJson()); });
        response.send(Http::Code::Ok, captures.dump());
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
                + host_header->port().toString() + request.resource());
    }

    return (std::nullopt);
}

static tl::expected<swagger::v1::model::PacketCapture, std::string>
parse_create_capture(const handler::request_type& request)
{
    try {
        return (nlohmann::json::parse(request.body())
                    .get<swagger::v1::model::PacketCapture>());
    } catch (const nlohmann::json::parse_error& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::create_captures(const request_type& request,
                              response_type response)
{
    auto capture = parse_create_capture(request);
    if (!capture) {
        response.send(Http::Code::Bad_Request, capture.error());
        return;
    }

    auto api_request = request_create_capture{
        std::make_unique<swagger::v1::model::PacketCapture>(
            std::move(*capture))};

    /* If the user provided an id, validate it before forwarding the request */
    if (!api_request.capture->getId().empty()) {
        auto res =
            config::op_config_validate_id_string(api_request.capture->getId());
        if (!res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* Make sure the capture object is valid before forwarding to the server */
    std::vector<std::string> errors;
    if (!is_valid(*api_request.capture, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_captures>(&api_reply)) {
        assert(reply->captures.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->captures[0]->getId());
        }

        response.send(Http::Code::Created, reply->captures[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_captures(const request_type&, response_type response)
{
    auto api_reply = submit_request(m_socket.get(), request_delete_captures{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_capture(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_capture{id});

    if (auto reply = std::get_if<reply_captures>(&api_reply)) {
        assert(reply->captures.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->captures[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_capture(const request_type& request,
                             response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_delete_capture{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::start_capture(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_start_capture{id});

    if (auto reply = std::get_if<reply_capture_results>(&api_reply)) {
        assert(reply->capture_results.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        if (auto uri = maybe_get_request_uri(request); uri.has_value()) {
            response.headers().add<Http::Header::Location>(
                *uri + reply->capture_results[0]->getId());
        }

        response.send(Http::Code::Created,
                      reply->capture_results[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::stop_capture(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_stop_capture{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<request_bulk_create_captures, std::string>
parse_bulk_create_captures(const handler::request_type& request)
{
    auto bulk_request = request_bulk_create_captures{};
    try {
        const auto j = nlohmann::json::parse(request.body());
        for (auto&& item : j["items"]) {
            bulk_request.captures.emplace_back(
                std::make_unique<capture_type>(item.get<capture_type>()));
        }
        return (bulk_request);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_create_captures(const request_type& request,
                                   response_type response)
{
    auto api_request = parse_bulk_create_captures(request);
    if (!api_request) {
        response.send(Http::Code::Bad_Request, api_request.error());
        return;
    }

    /* Verify that all user provided id's are valid */
    std::vector<std::string> id_errors;
    std::for_each(std::begin(api_request->captures),
                  std::end(api_request->captures),
                  [&](const auto& capture) {
                      if (!capture->getId().empty()) {
                          if (auto res = config::op_config_validate_id_string(
                                  capture->getId());
                              !res) {
                              id_errors.emplace_back(res.error());
                          }
                      }
                  });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    /* Validate all capture objects before forwarding to the server */
    std::vector<std::string> validation_errors;
    std::for_each(
        std::begin(api_request->captures),
        std::end(api_request->captures),
        [&](const auto& capture) { is_valid(*capture, validation_errors); });
    if (!validation_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(validation_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(*api_request));

    if (auto reply = std::get_if<reply_captures>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        auto swagger_reply =
            swagger::v1::model::BulkCreatePacketCapturesResponse{};
        std::move(std::begin(reply->captures),
                  std::end(reply->captures),
                  std::back_inserter(swagger_reply.getItems()));

        response.send(Http::Code::Ok, swagger_reply.toJson().dump());
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

void handler::bulk_delete_captures(const request_type& request,
                                   response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkDeletePacketCapturesRequest>(
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

    auto api_request = request_bulk_delete_captures{};

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

void handler::bulk_start_captures(const request_type& request,
                                  response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkStartPacketCapturesRequest>(
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

    auto api_request = request_bulk_start_captures{};

    std::transform(
        std::begin(ids),
        std::end(ids),
        std::back_inserter(api_request.ids),
        [](const auto& id) { return (std::make_unique<std::string>(id)); });

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_capture_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        auto swagger_reply =
            swagger::v1::model::BulkStartPacketCapturesResponse{};
        std::move(std::begin(reply->capture_results),
                  std::end(reply->capture_results),
                  std::back_inserter(swagger_reply.getItems()));

        response.send(Http::Code::Ok, swagger_reply.toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::bulk_stop_captures(const request_type& request,
                                 response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkStopPacketCapturesRequest>(
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

    auto api_request = request_bulk_stop_captures{};

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

void handler::list_capture_results(const request_type& request,
                                   response_type response)
{
    auto api_request = request_list_capture_results{};

    set_optional_filter(
        request, api_request.filter, filter_key_type::capture_id);

    set_optional_filter(
        request, api_request.filter, filter_key_type::source_id);

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_capture_results>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto capture_results = nlohmann::json::array();
        std::transform(std::begin(reply->capture_results),
                       std::end(reply->capture_results),
                       std::back_inserter(capture_results),
                       [](const auto& result) { return (result->toJson()); });
        response.send(Http::Code::Ok, capture_results.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_capture_results(const request_type&,
                                     response_type response)
{
    auto api_reply =
        submit_request(m_socket.get(), request_delete_capture_results{});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_capture_result(const request_type& request,
                                 response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_get_capture_result{id});

    if (auto reply = std::get_if<reply_capture_results>(&api_reply)) {
        assert(reply->capture_results.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok,
                      reply->capture_results[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_capture_result(const request_type& request,
                                    response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply =
        submit_request(m_socket.get(), request_delete_capture_result{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_capture_result_pcap(const request_type& request,
                                      response_type response)
{
    uint64_t packet_start = 0, packet_end = UINT64_MAX;
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    if (auto query = request.query().get("packet_start"); !query.isEmpty()) {
        auto v = to_uint64(query.get());
        if (!v) {
            auto err_msg =
                "packet start value is not valid (" + query.get() + ")";
            response.send(Http::Code::Bad_Request, err_msg);
            return;
        }
        packet_start = v.value();
    }
    if (auto query = request.query().get("packet_end"); !query.isEmpty()) {
        auto v = to_uint64(query.get());
        if (!v) {
            auto err_msg =
                "packet end value is not valid (" + query.get() + ")";
            response.send(Http::Code::Bad_Request, err_msg);
            return;
        }
        packet_end = v.value();
    }
    if (packet_start > packet_end) {
        auto err_msg = "packet start > end (" + std::to_string(packet_start)
                       + " > " + std::to_string(packet_end) + ")";
        response.send(Http::Code::Bad_Request, err_msg);
        return;
    }

    auto transfer_ptr =
        pcap::create_pcap_transfer_context(response, packet_start, packet_end);

    auto api_reply =
        submit_request(m_socket.get(),
                       request_create_capture_transfer{id, transfer_ptr.get()});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        // Request was successful so no longer own the transfer object
        auto transfer = transfer_ptr.release();
        auto result = pcap::send_pcap_response_header(response, *transfer);
        if (result.isRejected()) return;
        assert(result.isFulfilled());

        pcap::serve_capture_pcap(*transfer);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_capture_result_live(const request_type& request,
                                      response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto transfer_ptr = pcap::create_pcap_transfer_context(response);

    auto api_reply =
        submit_request(m_socket.get(),
                       request_create_capture_transfer{id, transfer_ptr.get()});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        // Request was successful so no longer own the transfer object
        auto transfer = transfer_ptr.release();
        auto result = pcap::send_pcap_response_header(response, *transfer);
        if (result.isRejected()) return;
        assert(result.isFulfilled());

        pcap::serve_capture_pcap(*transfer);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

} // namespace openperf::packet::capture::api
