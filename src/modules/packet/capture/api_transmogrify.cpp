#include "core/op_core.h"
#include "packet/capture/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketCapture.h"
#include "swagger/v1/model/PacketCaptureResult.h"

namespace openperf::packet::capture::api {

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
static auto zmq_msg_init(zmq_msg_t* msg, std::shared_ptr<T> value)
{
    using shared_ptr_type = std::shared_ptr<T>;

    // Allocate a shared_ptr to hold extra reference when passed over channel
    auto holder = std::make_unique<shared_ptr_type>(value);

    if (auto error = zmq_msg_init_size(msg, sizeof(shared_ptr_type*));
        error != 0) {
        return (error);
    }
    value.reset();

    auto ptr = reinterpret_cast<shared_ptr_type**>(zmq_msg_data(msg));
    *ptr = holder.release();
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
                           [&](request_list_captures& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(request.filter)));
                           },
                           [&](request_create_capture& request) {
                               return (
                                   zmq_msg_init(&serialized.data,
                                                std::move(request.capture)));
                           },
                           [&](const request_delete_captures&) {
                               return (zmq_msg_init(&serialized.data, 0));
                           },
                           [&](const request_get_capture& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_delete_capture& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](request_start_capture& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_stop_capture& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](request_list_capture_results& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    std::move(request.filter)));
                           },
                           [&](const request_delete_capture_results& request) {
                               return (zmq_msg_init(&serialized.data, 0));
                           },
                           [&](const request_get_capture_result& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_delete_capture_result& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_create_capture_reader& request) {
                               return (zmq_msg_init(&serialized.data,
                                                    request.id.data(),
                                                    request.id.length()));
                           },
                           [&](const request_delete_capture_reader& request) {
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
                    [&](reply_captures& reply) {
                        return (zmq_msg_init(&serialized.data, reply.captures));
                    },
                    [&](reply_capture_results& reply) {
                        return (zmq_msg_init(&serialized.data,
                                             reply.capture_results));
                    },
                    [&](reply_capture_reader& reply) {
                        return (zmq_msg_init(&serialized.data, reply.reader));
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
    case utils::variant_index<request_msg, request_list_captures>(): {
        auto request = request_list_captures{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_capture>(): {
        auto request = request_create_capture{};
        request.capture.reset(*zmq_msg_data<capture_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_captures>():
        return (request_delete_captures{});
    case utils::variant_index<request_msg, request_get_capture>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_capture{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_capture>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_capture{std::move(id)});
    }
    case utils::variant_index<request_msg, request_start_capture>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_start_capture{std::move(id)});
    }
    case utils::variant_index<request_msg, request_stop_capture>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_stop_capture{std::move(id)});
    }
    case utils::variant_index<request_msg, request_list_capture_results>(): {
        auto request = request_list_capture_results{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_capture_results>():
        return (request_delete_capture_results{});
    case utils::variant_index<request_msg, request_get_capture_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_capture_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_capture_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_capture_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_create_capture_reader>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_create_capture_reader{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_capture_reader>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_capture_reader{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = zmq_msg_data<index_type>(&msg.type);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_captures>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(capture_type*);
        auto data = zmq_msg_data<capture_type**>(&msg.data);

        auto reply = reply_captures{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.captures.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_capture_results>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(capture_result_type*);
        auto data = zmq_msg_data<capture_result_type**>(&msg.data);

        auto reply = reply_capture_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.capture_results.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_capture_reader>(): {
        auto holder_ptr = zmq_msg_data<std::shared_ptr<reader>**>(&msg.data);
        assert(holder_ptr);
        std::unique_ptr<std::shared_ptr<reader>> holder(*holder_ptr);
        auto reply = reply_capture_reader{*holder};
        return reply;
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

} // namespace openperf::packet::capture::api

namespace swagger::v1::model {
void from_json(const nlohmann::json& j, PacketCapture& capture)
{
    /*
     * We can't call capture's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        capture.setId(j["id"]);
    }

    if (j.find("source_id") != j.end() && !j["source_id"].is_null()) {
        capture.setSourceId(j["source_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        capture.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketCaptureConfig>();
        config->fromJson(const_cast<nlohmann::json&>(j["config"]));
        capture.setConfig(config);
    }
}

} // namespace swagger::v1::model
