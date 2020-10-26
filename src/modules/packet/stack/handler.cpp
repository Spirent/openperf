#include <zmq.h>

#include "api/api_route_handler.hpp"
#include "config/op_config_utils.hpp"
#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/stack/api.hpp"
#include "packetio/generic_interface.hpp"

#include "swagger/converters/packet_stack.hpp"
#include "swagger/v1/model/BulkCreateInterfacesRequest.h"
#include "swagger/v1/model/BulkCreateInterfacesResponse.h"
#include "swagger/v1/model/BulkDeleteInterfacesRequest.h"
#include "swagger/v1/model/Interface.h"
#include "swagger/v1/model/Stack.h"

namespace openperf::packet::stack::api {

class handler : public openperf::api::route::handler::registrar<handler>
{
public:
    handler(void* context, Pistache::Rest::Router& router);

    using request_type = Pistache::Rest::Request;
    using response_type = Pistache::Http::ResponseWriter;

    void list_interfaces(const request_type& request, response_type response);
    void create_interface(const request_type& request, response_type response);
    void get_interface(const request_type& request, response_type response);
    void delete_interface(const request_type& request, response_type response);
    void bulk_create_interfaces(const request_type& request,
                                response_type response);
    void bulk_delete_interfaces(const request_type& request,
                                response_type response);

    void list_stacks(const request_type& request, response_type response);
    void get_stack(const request_type& request, response_type response);

private:
    std::unique_ptr<void, op_socket_deleter> m_socket;
};

handler::handler(void* context, Pistache::Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, endpoint.data()))
{
    using namespace Pistache::Rest::Routes;

    Get(router, "/interfaces", bind(&handler::list_interfaces, this));
    Post(router, "/interfaces", bind(&handler::create_interface, this));
    Get(router, "/interfaces/:id", bind(&handler::get_interface, this));
    Delete(router, "/interfaces/:id", bind(&handler::delete_interface, this));

    Post(router,
         "/interfaces/x/bulk-create",
         bind(&handler::bulk_create_interfaces, this));
    Post(router,
         "/interfaces/x/bulk-delete",
         bind(&handler::bulk_delete_interfaces, this));

    Get(router, "/stacks", bind(&handler::list_stacks, this));
    Get(router, "/stacks/:id", bind(&handler::get_stack, this));
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

static std::string simple_url_decode(const std::string& input)
{
    std::ostringstream out;
    for (size_t i = 0; i < input.length(); i++) {
        if (input.at(i) == '%' && i + 2 < input.length()) {
            auto hex = std::strtol(input.substr(i + 1, 2).c_str(), nullptr, 16);
            out << static_cast<unsigned char>(hex);
            i += 2;
        } else {
            out << input.at(i);
        }
    }
    return out.str();
}

static void set_optional_filter(const handler::request_type& request,
                                intf_filter_map_ptr& filter,
                                intf_filter_type type)
{
    using namespace libpacket::type;

    if (auto query = request.query().get(to_string(type)); !query.isEmpty()) {
        if (!filter) { filter = std::make_unique<intf_filter_map_type>(); }
        switch (type) {
        case intf_filter_type::port_id:
            filter->emplace(type, simple_url_decode(query.get().data()));
            break;
        case intf_filter_type::eth_mac_address:
            filter->emplace(type,
                            mac_address(simple_url_decode(query.get().data())));
            break;
        case intf_filter_type::ipv4_address:
            filter->emplace(
                type, ipv4_address(simple_url_decode(query.get().data())));
            break;
        case intf_filter_type::ipv6_address:
            filter->emplace(
                type, ipv6_address(simple_url_decode(query.get().data())));
            break;
        default:
            break; /* only type left is 'none' */
        }
    }
}

void handler::list_interfaces(const request_type& request,
                              response_type response)
{
    auto api_request = request_list_interfaces{};

    /*
     * Setting the filter can throw a runtime error if the input cannot
     * be converted to the appropriate address type.
     */
    try {
        set_optional_filter(
            request, api_request.filter, intf_filter_type::port_id);
        set_optional_filter(
            request, api_request.filter, intf_filter_type::eth_mac_address);
        set_optional_filter(
            request, api_request.filter, intf_filter_type::ipv4_address);
        set_optional_filter(
            request, api_request.filter, intf_filter_type::ipv6_address);
    } catch (const std::runtime_error& error) {
        /*
         * If the user sends us garbage, then obviously it won't match anything
         * valid. Return an empty list.
         */
        OP_LOG(OP_LOG_WARNING,
               "Ignoring invalid interface filter: %s\n",
               error.what());
        response.send(Http::Code::Ok, nlohmann::json::array().dump());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_interfaces>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto interfaces = nlohmann::json::array();
        std::transform(std::begin(reply->interfaces),
                       std::end(reply->interfaces),
                       std::back_inserter(interfaces),
                       [](const auto& intf) { return (intf->toJson()); });
        response.send(Http::Code::Ok, interfaces.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<swagger::v1::model::Interface, std::string>
parse_create_interface(const handler::request_type& request)
{
    try {
        return (nlohmann::json::parse(request.body())
                    .get<swagger::v1::model::Interface>());
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::create_interface(const request_type& request,
                               response_type response)
{
    auto intf = parse_create_interface(request);
    if (!intf) {
        response.send(Http::Code::Bad_Request, intf.error());
        return;
    }

    auto api_request = request_create_interface{
        std::make_unique<swagger::v1::model::Interface>(std::move(*intf))};

    /* Verify id */
    if (!api_request.interface->getId().empty()) {
        auto res = config::op_config_validate_id_string(
            api_request.interface->getId());
        if (!res) {
            response.send(Http::Code::Not_Found, res.error());
            return;
        }
    }

    /* Verify interface configuration */
    auto errors = std::vector<std::string>{};
    if (!packetio::interface::is_valid(*api_request.interface, errors)) {
        fprintf(stderr, "request: 4a\n");
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_interfaces>(&api_reply)) {
        assert(reply->interfaces.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.headers().add<Http::Header::Location>(
            request.resource() + "/" + reply->interfaces[0]->getId());
        response.send(Http::Code::Created,
                      reply->interfaces[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_interface(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_interface{id});

    if (auto reply = std::get_if<reply_interfaces>(&api_reply)) {
        assert(reply->interfaces.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->interfaces[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::delete_interface(const request_type& request,
                               response_type response)
{
    OP_LOG(OP_LOG_DEBUG, "delete interface\n");
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    OP_LOG(OP_LOG_DEBUG, "deleting interface %s\n", id.c_str());
    auto api_reply =
        submit_request(m_socket.get(), request_delete_interface{id});

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

static tl::expected<request_bulk_create_interfaces, std::string>
parse_bulk_create_interfaces(const handler::request_type& request)
{
    auto bulk_request = request_bulk_create_interfaces{};
    try {
        const auto j = nlohmann::json::parse(request.body());
        for (const auto& item : j["items"]) {
            bulk_request.interfaces.emplace_back(
                std::make_unique<interface_type>(item.get<interface_type>()));
        }
        return (bulk_request);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_create_interfaces(const request_type& request,
                                     response_type response)
{
    auto api_request = parse_bulk_create_interfaces(request);
    if (!api_request) {
        response.send(Http::Code::Bad_Request, api_request.error());
        return;
    }

    auto id_errors = std::vector<std::string>{};
    std::for_each(std::begin(api_request->interfaces),
                  std::end(api_request->interfaces),
                  [&](const auto& intf) {
                      if (!intf->getId().empty()) {
                          if (auto res = config::op_config_validate_id_string(
                                  intf->getId());
                              !res) {
                              id_errors.emplace_back(res.error());
                          }
                      }
                  });
    if (!id_errors.empty()) {
        response.send(Http::Code::Not_Found, concatenate(id_errors));
        return;
    }

    /* Validate all interface objects before handing off to the server */
    auto validation_errors = std::vector<std::string>{};
    std::for_each(std::begin(api_request->interfaces),
                  std::end(api_request->interfaces),
                  [&](const auto& intf) {
                      packetio::interface::is_valid(*intf, validation_errors);
                  });
    if (!validation_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(validation_errors));
        return;
    }

    auto api_reply = submit_request(m_socket.get(), std::move(*api_request));

    if (auto reply = std::get_if<reply_interfaces>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));

        auto swagger_reply = swagger::v1::model::BulkCreateInterfacesResponse{};
        std::move(std::begin(reply->interfaces),
                  std::end(reply->interfaces),
                  std::back_inserter(swagger_reply.getItems()));

        response.send(Http::Code::Ok, swagger_reply.toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

template <typename T>
tl::expected<T, std::string> parse_request(const handler::request_type& request)
{
    auto obj = T{};
    const auto& body = request.body();
    if (body.empty()) return (obj);

    try {
        auto j = nlohmann::json::parse(body);
        obj.fromJson(j);
        return (obj);
    } catch (const nlohmann::json::exception& e) {
        return (tl::unexpected(json_error(e.id, e.what())));
    }
}

void handler::bulk_delete_interfaces(const request_type& request,
                                     response_type response)
{
    auto swagger_request =
        parse_request<swagger::v1::model::BulkDeleteInterfacesRequest>(request);
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

    auto api_request = request_bulk_delete_interfaces{ids};

    auto api_reply = submit_request(m_socket.get(), std::move(api_request));

    if (auto reply = std::get_if<reply_ok>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::list_stacks(const request_type&, response_type response)
{
    auto api_reply = submit_request(m_socket.get(), request_list_stacks{});

    if (auto reply = std::get_if<reply_stacks>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        auto stacks = nlohmann::json::array();
        std::transform(std::begin(reply->stacks),
                       std::end(reply->stacks),
                       std::back_inserter(stacks),
                       [](const auto& intf) { return (intf->toJson()); });
        response.send(Http::Code::Ok, stacks.dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

void handler::get_stack(const request_type& request, response_type response)
{
    auto id = request.param(":id").as<std::string>();
    if (auto res = config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Not_Found, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(), request_get_stack{id});

    if (auto reply = std::get_if<reply_stacks>(&api_reply)) {
        assert(reply->stacks.size() == 1);
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, reply->stacks[0]->toJson().dump());
    } else {
        handle_reply_error(api_reply, std::move(response));
    }
}

} // namespace openperf::packet::stack::api
