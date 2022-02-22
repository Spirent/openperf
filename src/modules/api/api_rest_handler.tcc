#ifndef _OP_API_REST_HANDLER_TCC
#define _OP_API_REST_HANDLER_TCC

#include <algorithm>
#include <variant>

#include "json.hpp"
#include "pistache/http.h"
#include "pistache/router.h"

#include "api/api_utils.hpp"
#include "config/op_config_utils.hpp"
#include "dynamic/api.hpp"
#include "swagger/v1/model/DynamicResultsConfig.h"

namespace openperf::api::rest {

using namespace Pistache;
using json = nlohmann::json;

static inline std::string concatenate(const std::vector<std::string>& strings)
{
    return (std::accumulate(
        std::begin(strings),
        std::end(strings),
        std::string{},
        [](std::string& lhs, const std::string& rhs) -> decltype(auto) {
            return (lhs += ((lhs.empty() ? "" : " ") + rhs));
        }));
}

static inline std::string json_error(int code, std::string_view message)
{
    return (
        nlohmann::json({{"code", code}, {"message", message.data()}}).dump());
}

template <typename, typename = std::void_t<>>
struct has_dynamic_results : std::false_type
{};

template <typename T>
struct has_dynamic_results<
    T,
    std::void_t<decltype(std::declval<T>().dynamic_results)>> : std::true_type
{};

template <typename T>
inline constexpr bool has_dynamic_results_v = has_dynamic_results<T>::value;

/**
 * Generic object functions
 */

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_list_objects(const Rest::Request&,
                     Http::ResponseWriter response,
                     RequestHandler&& on_request,
                     ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_list_type;
    using reply_type = typename Traits::reply_object_container_type;
    using extractor_type = typename Traits::object_extractor_type;

    auto api_reply = on_request(request_type{});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto objects = json::array();
        std::transform(std::begin(extractor_type{}(*reply)),
                       std::end(extractor_type{}(*reply)),
                       std::back_inserter(objects),
                       [](const auto& object) { return (object->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, objects);
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits,
          typename Object,
          typename RequestHandler,
          typename ErrorHandler>
void do_create_object(Object&& object,
                      Http::ResponseWriter response,
                      RequestHandler&& on_request,
                      ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_create_type;
    using reply_type = typename Traits::reply_object_container_type;
    using extractor_type = typename Traits::object_extractor_type;
    using validator_type = typename Traits::object_validator_type;

    if (!object->getId().empty()) {
        auto res = config::op_config_validate_id_string(object->getId());
        if (!res) { response.send(Http::Code::Bad_Request, res.error()); }
    }

    std::vector<std::string> errors;
    if (!validator_type{}(*object, errors)) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = on_request(request_type{std::move(object)});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto& objects = extractor_type{}(*reply);
        assert(!objects.empty());
        response.headers().add<Http::Header::Location>(
            Traits::object_path_prefix + objects.front()->getId());
        openperf::api::utils::send_chunked_response(std::move(response),
                                                    Http::Code::Created,
                                                    objects.front()->toJson());
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_get_object(const Rest::Request& request,
                   Http::ResponseWriter response,
                   RequestHandler&& on_request,
                   ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_get_type;
    using reply_type = typename Traits::reply_object_container_type;
    using extractor_type = typename Traits::object_extractor_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = on_request(request_type{{id}});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto& objects = extractor_type{}(*reply);
        assert(!objects.empty());
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, objects.front()->toJson());
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_delete_object(const Rest::Request& request,
                      Http::ResponseWriter response,
                      RequestHandler&& on_request,
                      ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_delete_type;
    using reply_type = typename Traits::reply_ok_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = on_request(request_type{{id}});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_start_object(const Rest::Request& request,
                     Http::ResponseWriter response,
                     RequestHandler&& on_request,
                     ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_start_type;
    using reply_type = typename Traits::reply_result_container_type;
    using extractor_type = typename Traits::result_extractor_type;
    using dynamic_result_validator =
        typename Traits::dynamic_result_validator_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_request = request_type{.id = id};

    if constexpr (has_dynamic_results_v<request_type>) {
        if (!request.body().empty()) {
            auto dynamic_config = swagger::v1::model::DynamicResultsConfig();
            auto j = json::parse(request.body());
            dynamic_config.fromJson(j);
            auto errors = std::vector<std::string>{};
            if (!dynamic_result_validator{}(dynamic_config, errors)) {
                response.send(Http::Code::Bad_Request, concatenate(errors));
                return;
            }
            api_request.dynamic_results = dynamic::from_swagger(dynamic_config);
        }
    }

    auto api_reply = on_request(std::move(api_request));

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto& results = extractor_type{}(*reply);
        assert(!results.empty());
        response.headers().add<Http::Header::Location>(
            Traits::result_path_prefix + reply->results.front()->getId());
        openperf::api::utils::send_chunked_response(std::move(response),
                                                    Http::Code::Created,
                                                    results.front()->toJson());
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_stop_object(const Rest::Request& request,
                    Http::ResponseWriter response,
                    RequestHandler&& on_request,
                    ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_stop_type;
    using reply_type = typename Traits::reply_ok_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = on_request(request_type{{id}});

    if (std::get_if<reply_type>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        on_error(api_reply, std::move(response));
    }
}

/**
 * Generic result functions
 */

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_list_results(const Rest::Request&,
                     Http::ResponseWriter response,
                     RequestHandler&& on_request,
                     ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_result_list_type;
    using reply_type = typename Traits::reply_result_container_type;
    using extractor_type = typename Traits::result_extractor_type;

    auto api_reply = on_request(request_type{});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto results = json::array();
        std::transform(std::begin(extractor_type{}(*reply)),
                       std::end(extractor_type{}(*reply)),
                       std::back_inserter(results),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, results);
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_get_result(const Rest::Request& request,
                   Http::ResponseWriter response,
                   RequestHandler&& on_request,
                   ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_result_get_type;
    using reply_type = typename Traits::reply_result_container_type;
    using extractor_type = typename Traits::result_extractor_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = on_request(request_type{{id}});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto& results = extractor_type{}(*reply);
        assert(!results.empty());
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, results.front()->toJson());
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits, typename RequestHandler, typename ErrorHandler>
void do_delete_result(const Rest::Request& request,
                      Http::ResponseWriter response,
                      RequestHandler&& on_request,
                      ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_result_delete_type;
    using reply_type = typename Traits::reply_ok_type;

    auto id = request.param(":id").as<std::string>();
    if (auto res = openperf::config::op_config_validate_id_string(id); !res) {
        response.send(Http::Code::Bad_Request, res.error());
        return;
    }

    auto api_reply = on_request(request_type{{id}});

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        on_error(api_reply, std::move(response));
    }
}

/**
 * Bulk operations
 */

template <typename Traits,
          typename CreateRequest,
          typename RequestHandler,
          typename ErrorHandler>
void do_bulk_create_objects(CreateRequest&& request,
                            Http::ResponseWriter response,
                            RequestHandler&& on_request,
                            ErrorHandler&& on_error)
{
    using reply_type = typename Traits::reply_object_container_type;
    using extractor_type = typename Traits::object_extractor_type;
    using validator_type = typename Traits::object_validator_type;

    std::vector<std::string> errors;
    const auto& objects = extractor_type{}(request);
    std::for_each(
        std::begin(objects), std::end(objects), [&](const auto& object) {
            if (object->getId().empty()) { return; }
            if (auto res =
                    config::op_config_validate_id_string(object->getId());
                !res) {
                errors.emplace_back(res.error());
            }
        });
    if (!errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    std::for_each(
        std::begin(objects), std::end(objects), [&](const auto& object) {
            validator_type{}(*object, errors);
        });
    if (!errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(errors));
        return;
    }

    auto api_reply = on_request(std::move(request));

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto objects = json::array();
        std::transform(std::begin(extractor_type{}(*reply)),
                       std::end(extractor_type{}(*reply)),
                       std::back_inserter(objects),
                       [](const auto& object) { return (object->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, objects);
    } else {
        on_error(api_reply, std::move(response));
    }
}

/*
 * Work around the swagger type's lack of a const version of getIds()
 */
template <typename Request> const auto& get_ids(const Request& request)
{
    return (const_cast<Request&>(request).getIds());
}

template <typename Traits,
          typename SwaggerRequest,
          typename RequestHandler,
          typename ErrorHandler>
void do_bulk_start_objects(const SwaggerRequest& request,
                           Http::ResponseWriter response,
                           RequestHandler&& on_request,
                           ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_bulk_start_type;
    using reply_type = typename Traits::reply_result_container_type;
    using extractor_type = typename Traits::result_extractor_type;
    using dynamic_result_validator =
        typename Traits::dynamic_result_validator_type;

    /* Verify that all user provided id's are valid */
    const auto& ids = get_ids(request);
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (auto res = openperf::config::op_config_validate_id_string(id);
            !res) {
            id_errors.emplace_back(res.error());
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    auto api_request = request_type{.ids = ids};

    if constexpr (has_dynamic_results_v<request_type>) {
        if (request.dynamicResultsIsSet()) {
            auto errors = std::vector<std::string>{};
            if (!dynamic_result_validator{}(*(request.getDynamicResults()),
                                            errors)) {
                response.send(Http::Code::Bad_Request, concatenate(errors));
                return;
            }
            api_request.dynamic_results =
                dynamic::from_swagger(*(request.getDynamicResults()));
        }
    }

    auto api_reply = on_request(std::move(api_request));

    if (auto* reply = std::get_if<reply_type>(&api_reply)) {
        auto results = json::array();
        std::transform(std::begin(extractor_type{}(*reply)),
                       std::end(extractor_type{}(*reply)),
                       std::back_inserter(results),
                       [](const auto& result) { return (result->toJson()); });
        openperf::api::utils::send_chunked_response(
            std::move(response), Http::Code::Ok, results);
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits,
          typename SwaggerRequest,
          typename RequestHandler,
          typename ErrorHandler>
void do_bulk_stop_objects(const SwaggerRequest& request,
                          Http::ResponseWriter response,
                          RequestHandler&& on_request,
                          ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_bulk_stop_type;
    using reply_type = typename Traits::reply_ok_type;

    /* Verify that all user provided id's are valid */
    const auto& ids = get_ids(request);
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (auto res = config::op_config_validate_id_string(id); !res) {
            id_errors.emplace_back(res.error());
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    auto api_reply = on_request(request_type{{ids}});

    if (std::get_if<reply_type>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        on_error(api_reply, std::move(response));
    }
}

template <typename Traits,
          typename SwaggerRequest,
          typename RequestHandler,
          typename ErrorHandler>
void do_bulk_delete_objects(const SwaggerRequest& request,
                            Http::ResponseWriter response,
                            RequestHandler&& on_request,
                            ErrorHandler&& on_error)
{
    using request_type = typename Traits::request_object_bulk_delete_type;
    using reply_type = typename Traits::reply_ok_type;

    /* Verify that all user provided id's are valid */
    const auto& ids = get_ids(request);
    std::vector<std::string> id_errors;
    std::for_each(std::begin(ids), std::end(ids), [&](const auto& id) {
        if (auto res = config::op_config_validate_id_string(id); !res) {
            id_errors.emplace_back(res.error());
        }
    });
    if (!id_errors.empty()) {
        response.send(Http::Code::Bad_Request, concatenate(id_errors));
        return;
    }

    auto api_reply = on_request(request_type{{ids}});

    if (std::get_if<reply_type>(&api_reply)) {
        response.send(Http::Code::No_Content);
    } else {
        on_error(api_reply, std::move(response));
    }
}

} // namespace openperf::api::rest

#endif /* _OP_API_REST_HANDLER_TCC */
