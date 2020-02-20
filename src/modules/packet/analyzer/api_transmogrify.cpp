#include "core/op_core.h"
#include "packet/analyzer/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/Analyzer.h"
#include "swagger/v1/model/AnalyzerResult.h"
#include "swagger/v1/model/RxStream.h"

namespace openperf::packet::analyzer::api {

template <typename T> static auto zmq_msg_init(zmq_msg_t* msg, const T& value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T*>(zmq_msg_data(msg));
    *ptr = value;
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, std::unique_ptr<T> value)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*)); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<T**>(zmq_msg_data(msg));
    *ptr = value.release();
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg, const T* buffer, size_t length)
{
    if (auto error = zmq_msg_init_size(msg, length); error != 0) {
        return (error);
    }

    auto ptr = reinterpret_cast<char*>(zmq_msg_data(msg));
    auto src = reinterpret_cast<const char*>(buffer);
    std::copy(src, src + length, ptr);
    return (0);
}

template <typename T>
static auto zmq_msg_init(zmq_msg_t* msg,
                         std::vector<std::unique_ptr<T>>& values)
{
    if (auto error = zmq_msg_init_size(msg, sizeof(T*) * values.size());
        error != 0) {
        return (error);
    }

    auto cursor = reinterpret_cast<T**>(zmq_msg_data(msg));
    std::transform(std::begin(values), std::end(values), cursor, [](auto& ptr) {
        return (ptr.release());
    });
    return (0);
}

template <typename T>
static std::enable_if_t<!std::is_pointer_v<T>, T>
zmq_msg_data(const zmq_msg_t* msg)
{
    return *(reinterpret_cast<T*>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

template <typename T>
static std::enable_if_t<std::is_pointer_v<T>, T>
zmq_msg_data(const zmq_msg_t* msg)
{
    return (reinterpret_cast<T>(zmq_msg_data(const_cast<zmq_msg_t*>(msg))));
}

std::vector<char> serialize(const std::vector<std::string>& src)
{
    static constexpr char delimiter = '\n';
    auto dst = std::vector<char>{};
    std::for_each(std::begin(src), std::end(src), [&](const auto& s) {
        std::copy(std::begin(s), std::end(s), std::back_inserter(dst));
        dst.push_back(delimiter);
    });
    return (dst);
}

std::vector<std::string> deserialize(const std::vector<char>& src)
{
    static constexpr std::array<char, 1> delimiters = {'\n'};
    std::vector<std::string> dst;
    auto cursor = std::begin(src);
    while (cursor != std::end(src)) {
        auto token = std::find_first_of(cursor,
                                        std::end(src),
                                        std::begin(delimiters),
                                        std::end(delimiters));
        dst.emplace_back(*cursor, std::distance(cursor, token));
    }
    return (dst);
}

static void close(serialized_msg& msg)
{
    zmq_close(&msg.type);
    zmq_close(&msg.data);
}

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](request_list_analyzers& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(request.filter)));
                           },
                           [&](request_create_analyzer& request) {
                               return (
                                   zmq_msg_init(&serialized.data,
                                                std::move(request.analyzer)));
                           },
                           [&](const request_delete_analyzers&) {
                               return (zmq_msg_init(&serialized.data, 0));
                           },
                           [&](const request_get_analyzer& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_delete_analyzer& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](request_start_analyzer& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_stop_analyzer& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](request_list_analyzer_results& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(request.filter)));
                           },
                           [&](const request_delete_analyzer_results& request) {
                               return (zmq_msg_init(&serialized.data, 0));
                           },
                           [&](const request_get_analyzer_result& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_delete_analyzer_result& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](request_list_rx_streams& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(request.filter)));
                           },
                           [&](const request_get_rx_stream& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (zmq_msg_init(&serialized.type, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](reply_analyzers& reply) {
                     return (zmq_msg_init(&serialized.data, reply.analyzers));
                 },
                 [&](reply_analyzer_results& reply) {
                     return (zmq_msg_init(&serialized.data,
                                          reply.analyzer_results));
                 },
                 [&](reply_rx_streams& reply) {
                     return (zmq_msg_init(&serialized.data, reply.streams));
                 },
                 [&](const reply_ok&) {
                     return (zmq_msg_init(&serialized.data, 0));
                 },
                 [&](const reply_error& error) {
                     return (zmq_msg_init(&serialized.data, error.info));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = zmq_msg_data<index_type>(&msg.type);
    switch (idx) {
    case utils::variant_index<request_msg, request_list_analyzers>(): {
        auto request = request_list_analyzers{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_analyzer>(): {
        auto request = request_create_analyzer{};
        request.analyzer.reset(*zmq_msg_data<analyzer_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_analyzers>():
        return (request_delete_analyzers{});
    case utils::variant_index<request_msg, request_get_analyzer>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_analyzer{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_analyzer>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_analyzer{std::move(id)});
    }
    case utils::variant_index<request_msg, request_start_analyzer>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_start_analyzer{std::move(id)});
    }
    case utils::variant_index<request_msg, request_stop_analyzer>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_stop_analyzer{std::move(id)});
    }
    case utils::variant_index<request_msg, request_list_analyzer_results>(): {
        auto request = request_list_analyzer_results{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_analyzer_results>():
        return (request_delete_analyzer_results{});
    case utils::variant_index<request_msg, request_get_analyzer_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_analyzer_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_analyzer_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_analyzer_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_list_rx_streams>(): {
        auto request = request_list_rx_streams{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_rx_stream>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_rx_stream{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = zmq_msg_data<index_type>(&msg.type);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_analyzers>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(analyzer_type*);
        auto data = zmq_msg_data<analyzer_type**>(&msg.data);

        auto reply = reply_analyzers{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.analyzers.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_analyzer_results>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(analyzer_result_type*);
        auto data = zmq_msg_data<analyzer_result_type**>(&msg.data);

        auto reply = reply_analyzer_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.analyzer_results.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_rx_streams>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(rx_stream_type*);
        auto data = zmq_msg_data<rx_stream_type**>(&msg.data);

        auto reply = reply_rx_streams{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.streams.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{zmq_msg_data<typed_error>(&msg.data)});
    }

    return (tl::make_unexpected(EINVAL));
}

int send_message(void* socket, serialized_msg&& msg)
{
    if (zmq_msg_send(&msg.type, socket, ZMQ_SNDMORE) == -1
        || zmq_msg_send(&msg.data, socket, 0) == -1) {
        close(msg);
        return (errno);
    }

    return (0);
}

tl::expected<serialized_msg, int> recv_message(void* socket, int flags)
{
    serialized_msg msg;
    if (zmq_msg_init(&msg.type) == -1 || zmq_msg_init(&msg.data) == -1) {
        close(msg);
        return (tl::make_unexpected(ENOMEM));
    }

    if (zmq_msg_recv(&msg.type, socket, flags) == -1
        || zmq_msg_recv(&msg.data, socket, flags) == -1) {
        close(msg);
        return (tl::make_unexpected(errno));
    }

    return (msg);
}

reply_error to_error(error_type type, int value)
{
    return (reply_error{.info = typed_error{.type = type, .value = value}});
}

} // namespace openperf::packet::analyzer::api

namespace swagger::v1::model {
void from_json(const nlohmann::json& j, Analyzer& analyzer)
{
    /*
     * We can't call analyzer's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        analyzer.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        analyzer.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        analyzer.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<AnalyzerConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        analyzer.setConfig(config);
    }
}

} // namespace swagger::v1::model
