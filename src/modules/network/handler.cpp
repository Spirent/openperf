#include "api.hpp"
#include "api_converters.hpp"

#include "framework/config/op_config_utils.hpp"
#include "framework/core/op_core.h"
#include "framework/message/serialized_message.hpp"
#include "modules/api/api_route_handler.hpp"
#include "modules/dynamic/api.hpp"

#include "swagger/converters/network.hpp"

namespace openperf::network {

using namespace Pistache;
using namespace swagger::v1::model;
using json = nlohmann::json;

class handler : public openperf::api::route::handler::registrar<handler>
{
    std::unique_ptr<void, op_socket_deleter> m_socket;

public:
    handler(void* context, Rest::Router& router);

    void list_servers(const Rest::Request& request,
                      Http::ResponseWriter response);

    void create_server(const Rest::Request& request,
                       Http::ResponseWriter response);

    void get_server(const Rest::Request& request,
                    Http::ResponseWriter response);

    void delete_server(const Rest::Request& request,
                       Http::ResponseWriter response);

    void bulk_create_servers(const Rest::Request& request,
                             Http::ResponseWriter response);

    void bulk_delete_servers(const Rest::Request& request,
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

enum Http::Code to_code(const api::reply::error& error)
{
    switch (error.info->type) {
    case api::error_type::NOT_FOUND:
        return (Http::Code::Not_Found);
    case api::error_type::CUSTOM_ERROR:
        return (Http::Code::Bad_Request);
    default:
        return (Http::Code::Internal_Server_Error);
    }
}

handler::handler(void* context, Rest::Router& router)
    : m_socket(op_socket_get_client(context, ZMQ_REQ, api::endpoint.data()))
{
    // Rest::Routes::Get(router,
    //                   "/network/servers",
    //                   Rest::Routes::bind(&handler::list_servers, this));
    // Rest::Routes::Post(router,
    //                    "/network/servers",
    //                    Rest::Routes::bind(&handler::create_server, this));
    // Rest::Routes::Get(router,
    //                   "/network/servers/:id",
    //                   Rest::Routes::bind(&handler::get_server, this));
    // Rest::Routes::Delete(router,
    //                      "/network/servers/:id",
    //                      Rest::Routes::bind(&handler::delete_server, this));
    // Rest::Routes::Post(router,
    //                    "/network/servers/x/bulk-create",
    //                    Rest::Routes::bind(&handler::bulk_create_servers,
    //                    this));
    // Rest::Routes::Post(router,
    //                    "/network/servers/x/bulk-delete",
    //                    Rest::Routes::bind(&handler::bulk_delete_servers,
    //                    this));
    // Rest::Routes::Get(router,
    //                   "/network/generators",
    //                   Rest::Routes::bind(&handler::list_generators, this));
    // Rest::Routes::Post(router,
    //                    "/network/generators",
    //                    Rest::Routes::bind(&handler::create_generator, this));
    // Rest::Routes::Get(router,
    //                   "/network/generators/:id",
    //                   Rest::Routes::bind(&handler::get_generator, this));
    // Rest::Routes::Delete(router,
    //                      "/network/generators/:id",
    //                      Rest::Routes::bind(&handler::delete_generator,
    //                      this));
    // Rest::Routes::Post(
    //     router,
    //     "/network/generators/x/bulk-create",
    //     Rest::Routes::bind(&handler::bulk_create_generators, this));
    // Rest::Routes::Post(
    //     router,
    //     "/network/generators/x/bulk-delete",
    //     Rest::Routes::bind(&handler::bulk_delete_generators, this));
    // Rest::Routes::Post(router,
    //                    "/network/generators/:id/start",
    //                    Rest::Routes::bind(&handler::start_generator, this));
    // Rest::Routes::Post(router,
    //                    "/network/generators/:id/stop",
    //                    Rest::Routes::bind(&handler::stop_generator, this));
    // Rest::Routes::Post(
    //     router,
    //     "/network/generators/x/bulk-start",
    //     Rest::Routes::bind(&handler::bulk_start_generators, this));
    // Rest::Routes::Post(
    //     router,
    //     "/network/generators/x/bulk-stop",
    //     Rest::Routes::bind(&handler::bulk_stop_generators, this));
    // Rest::Routes::Get(
    //     router,
    //     "/network/generator-results",
    //     Rest::Routes::bind(&handler::list_generator_results, this));
    // Rest::Routes::Get(router,
    //                   "/network/generator-results/:id",
    //                   Rest::Routes::bind(&handler::get_generator_result,
    //                   this));
    // Rest::Routes::Delete(
    //     router,
    //     "/network/generator-results/:id",
    //     Rest::Routes::bind(&handler::delete_generator_result, this));
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

// void handler::list_servers(const Rest::Request&, Http::ResponseWriter
// response)
// {
//     auto api_reply =
//         submit_request(m_socket.get(), api::request_network_server_list{});
//     if (auto reply = std::get_if<api::reply_network_servers>(&api_reply)) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         auto servers = json::array();
//         std::transform(std::begin(reply->servers),
//                        std::end(reply->servers),
//                        std::back_inserter(servers),
//                        [](const auto& blkserver) {
//                            return (api::to_swagger(*blkserver)->toJson());
//                        });
//         response.send(Http::Code::Ok, servers.dump());
//     } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//         response.send(to_code(*error), api::to_string(error->info));
//     } else {
//         response.send(Http::Code::Internal_Server_Error);
//     }
// }

// void handler::create_server(const Rest::Request& request,
//                             Http::ResponseWriter response)
// {
//     try {
//         auto server_model = json::parse(request.body()).get<NetworkServer>();

//         /* Make sure the object is valid before submitting request */
//         std::vector<std::string> errors;
//         if (!api::is_valid(server_model, errors)) {
//             response.send(Http::Code::Bad_Request, concatenate(errors));
//             return;
//         }

//         auto api_reply = submit_request(
//             m_socket.get(),
//             api::request_network_server_add{
//                 std::make_unique<openperf::network::model::server>(
//                     api::from_swagger(server_model))});
//         if (auto reply = std::get_if<api::reply_network_servers>(&api_reply))
//         {
//             assert(!reply->servers.empty());
//             response.headers().add<Http::Header::ContentType>(
//                 MIME(Application, Json));
//             response.headers().add<Http::Header::Location>(
//                 request.resource() + "/" + reply->servers.front()->id());
//             response.send(
//                 Http::Code::Created,
//                 api::to_swagger(*reply->servers.front())->toJson().dump());
//         } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//             response.send(to_code(*error), api::to_string(error->info));
//         } else {
//             response.send(Http::Code::Internal_Server_Error);
//         }
//     } catch (const json::exception& e) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         response.send(
//             Http::Code::Bad_Request,
//             nlohmann::json({{"code", e.id}, {"message", e.what()}}).dump());
//     }
// }

// void handler::get_server(const Rest::Request& request,
//                          Http::ResponseWriter response)
// {
//     auto id = request.param(":id").as<std::string>();
//     if (auto res = openperf::config::op_config_validate_id_string(id); !res)
//     {
//         response.send(Http::Code::Not_Found, res.error());
//         return;
//     }

//     auto api_reply =
//         submit_request(m_socket.get(), api::request_network_server{id : id});
//     if (auto reply = std::get_if<api::reply_network_servers>(&api_reply)) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         response.send(
//             Http::Code::Ok,
//             api::to_swagger(*reply->servers.front())->toJson().dump());
//     } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//         response.send(to_code(*error), api::to_string(error->info));
//     } else {
//         response.send(Http::Code::Internal_Server_Error);
//     }
// }

// void handler::delete_server(const Rest::Request& request,
//                             Http::ResponseWriter response)
// {
//     auto id = request.param(":id").as<std::string>();
//     if (auto res = openperf::config::op_config_validate_id_string(id); !res)
//     {
//         response.send(Http::Code::Not_Found);
//         return;
//     }

//     auto api_reply = submit_request(m_socket.get(),
//                                     api::request_network_server_del{id :
//                                     id});
//     if (auto reply = std::get_if<api::reply_ok>(&api_reply)) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         response.send(Http::Code::No_Content);
//     } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//         response.send(to_code(*error), api::to_string(error->info));
//     } else {
//         response.send(Http::Code::Internal_Server_Error);
//     }
// }

// void handler::bulk_create_servers(const Rest::Request& request,
//                                   Http::ResponseWriter response)
// {
//     auto request_model =
//         json::parse(request.body()).get<BulkCreatenetworkServersRequest>();

//     auto api_reply =
//         submit_request(m_socket.get(), api::from_swagger(request_model));
//     if (auto reply = std::get_if<api::reply_network_servers>(&api_reply)) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         auto results = json::array();
//         std::transform(std::begin(reply->servers),
//                        std::end(reply->servers),
//                        std::back_inserter(results),
//                        [](const auto& result) {
//                            return (api::to_swagger(*result)->toJson());
//                        });
//         response.send(Http::Code::Ok, results.dump());
//     } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//         response.send(to_code(*error), api::to_string(error->info));
//     } else {
//         response.send(Http::Code::Internal_Server_Error);
//     }
// }

// void handler::bulk_delete_servers(const Rest::Request& request,
//                                   Http::ResponseWriter response)
// {
//     auto request_model =
//         json::parse(request.body()).get<BulkDeleteNetworkServersRequest>();

//     auto api_reply =
//         submit_request(m_socket.get(), api::from_swagger(request_model));
//     if (std::get_if<api::reply_ok>(&api_reply)) {
//         response.headers().add<Http::Header::ContentType>(
//             MIME(Application, Json));
//         response.send(Http::Code::No_Content);
//     } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
//         response.send(to_code(*error), api::to_string(error->info));
//     } else {
//         response.send(Http::Code::Internal_Server_Error);
//     }
// }

void handler::list_generators(const Rest::Request&,
                              Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request::generator::list{});
    if (auto reply = std::get_if<api::reply::generator::list>(&api_reply)) {
        auto generators = json::array();
        std::transform(std::begin(reply->generators),
                       std::end(reply->generators),
                       std::back_inserter(generators),
                       [](const auto& generator) {
                           return api::to_swagger(*generator)->toJson();
                       });

        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, generators.dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::create_generator(const Rest::Request& request,
                               Http::ResponseWriter response)
{
    try {
        auto generator_model =
            json::parse(request.body()).get<NetworkGenerator>();

        if (auto ok = api::is_valid(*generator_model.getConfig()); !ok) {
            throw json::other_error::create(
                0,
                std::accumulate(
                    ok.error().begin(),
                    ok.error().end(),
                    std::string{},
                    [](auto& acc, auto& s) { return acc += " " + s; })
                    .c_str());
        }

        auto api_request = api::request::generator::create{
            std::make_unique<api::network_generator_t>(
                api::from_swagger(generator_model))};
        auto api_reply = submit_request(m_socket.get(), std::move(api_request));
        if (auto reply = std::get_if<api::reply::generator::list>(&api_reply)) {
            assert(!reply->generators.empty());
            response.headers().add<Http::Header::Location>(
                "/network/generators/" + reply->generators.front()->id());
            response.headers().add<Http::Header::ContentType>(
                MIME(Application, Json));
            response.send(
                Http::Code::Created,
                api::to_swagger(*reply->generators.front())->toJson().dump());
        } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
            response.send(to_code(*error), api::to_string(*error->info));
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
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = submit_request(m_socket.get(),
                                    api::request::generator::get{{.id = id}});
    if (auto reply = std::get_if<api::reply::generator::list>(&api_reply)) {
        assert(!reply->generators.empty());
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Ok,
            api::to_swagger(*reply->generators.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    auto api_reply = submit_request(m_socket.get(),
                                    api::request::generator::erase{{.id = id}});

    if (std::get_if<api::reply::ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_create_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkCreateNetworkGeneratorsRequest>();

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (auto reply = std::get_if<api::reply::generator::list>(&api_reply)) {
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
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_delete_generators(const Rest::Request& request,
                                     Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkDeleteNetworkGeneratorsRequest>();

    auto api_reply =
        submit_request(m_socket.get(), api::from_swagger(request_model));
    if (std::get_if<api::reply::ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    api::request::generator::start data{.id = id};

    if (!request.body().empty()) {
        auto json_obj = json::parse(request.body());
        DynamicResultsConfig model;
        model.fromJson(json_obj);

        data.dynamic_results = dynamic::from_swagger(model);
    }

    auto api_reply = submit_request(m_socket.get(), std::move(data));

    if (auto reply = std::get_if<api::reply::statistic::list>(&api_reply)) {
        assert(!reply->results.empty());
        response.headers().add<Http::Header::Location>(
            "/network/generator-results/" + reply->results.front()->id());
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Created,
            api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    auto api_reply = submit_request(m_socket.get(),
                                    api::request::generator::stop{{.id = id}});
    if (std::get_if<api::reply::ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_start_generators(const Rest::Request& request,
                                    Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkStartNetworkGeneratorsRequest>();

    for (const auto& id : request_model.getIds()) {
        if (auto r = openperf::config::op_config_validate_id_string(id); !r) {
            response.send(Http::Code::Bad_Request, r.error());
            return;
        }
    }

    api::request::generator::bulk::start data{
        .ids = std::move(request_model.getIds())};

    if (request_model.dynamicResultsIsSet())
        data.dynamic_results =
            dynamic::from_swagger(*request_model.getDynamicResults().get());

    auto api_reply = submit_request(m_socket.get(), std::move(data));

    if (auto reply = std::get_if<api::reply::statistic::list>(&api_reply)) {
        auto results = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return api::to_swagger(*result)->toJson();
                       });

        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::bulk_stop_generators(const Rest::Request& request,
                                   Http::ResponseWriter response)
{
    auto request_model =
        json::parse(request.body()).get<BulkStopNetworkGeneratorsRequest>();

    api::request::generator::bulk::stop bulk_request{};
    for (const auto& id : request_model.getIds()) {
        if (auto r = openperf::config::op_config_validate_id_string(id); !r) {
            response.send(Http::Code::Bad_Request, r.error());
            return;
        }
        bulk_request.ids.push_back(std::make_unique<std::string>(id));
    }

    auto api_reply = submit_request(m_socket.get(), std::move(bulk_request));
    if (std::get_if<api::reply::ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

void handler::list_generator_results(const Rest::Request&,
                                     Http::ResponseWriter response)
{
    auto api_reply =
        submit_request(m_socket.get(), api::request::statistic::list{});
    if (auto reply = std::get_if<api::reply::statistic::list>(&api_reply)) {
        auto results = json::array();
        std::transform(std::begin(reply->results),
                       std::end(reply->results),
                       std::back_inserter(results),
                       [](const auto& result) {
                           return api::to_swagger(*result)->toJson();
                       });

        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::Ok, results.dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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

    auto api_reply = submit_request(m_socket.get(),
                                    api::request::statistic::get{{.id = id}});
    if (auto reply = std::get_if<api::reply::statistic::list>(&api_reply)) {
        assert(!reply->results.empty());
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(
            Http::Code::Ok,
            api::to_swagger(*reply->results.front())->toJson().dump());
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
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
                                    api::request::statistic::erase{{.id = id}});
    if (std::get_if<api::reply::ok>(&api_reply)) {
        response.headers().add<Http::Header::ContentType>(
            MIME(Application, Json));
        response.send(Http::Code::No_Content);
    } else if (auto error = std::get_if<api::reply::error>(&api_reply)) {
        response.send(to_code(*error), api::to_string(*error->info));
    } else {
        response.send(Http::Code::Internal_Server_Error);
    }
}

} // namespace openperf::network
