#ifndef _OP_PACKET_CAPTURE_API_HPP_
#define _OP_PACKET_CAPTURE_API_HPP_

#include <string>
#include <memory>
#include <variant>
#include <vector>

#include <zmq.h>

#include "json.hpp"
#include "tl/expected.hpp"

namespace swagger::v1::model {

class PacketCapture;
class PacketCaptureResult;

} // namespace swagger::v1::model

namespace openperf::core {

class uuid;

} // namespace openperf::core

namespace openperf::message {
struct serialized_message;
}

namespace openperf::packet::capture {

class sink;
struct sink_result;
class transfer_context;
class capture_buffer_reader;

class transfer_context
{
public:
    virtual ~transfer_context() = default;
    virtual void set_reader(std::unique_ptr<capture_buffer_reader>& reader) = 0;
    virtual bool is_done() const = 0;
    virtual void set_done_callback(std::function<void()>&& callback) = 0;
};

namespace api {

inline constexpr std::string_view endpoint = "inproc://openperf_packet_capture";

/*
 * Provide some sane typedefs for the swagger types we deal with.
 *
 * Note: this API is based around moving pointers between the REST API
 * handler and the server backend that actually manages capture resources.
 * Strings and vectors of strings are trivial to serialize and can be handled
 * directly. More complex types are wrapped in a std::unique_ptr so that proper
 * ownership semantics can be enforced when objects are shipped across the
 * channel. The serialization/de-serialization routines will release and reset
 * the smart pointers as appropriate and send the raw pointer across the
 * channel.
 */

using capture_type = swagger::v1::model::PacketCapture;
using capture_ptr = std::unique_ptr<capture_type>;

using capture_result_type = swagger::v1::model::PacketCaptureResult;
using capture_result_ptr = std::unique_ptr<capture_result_type>;

enum class filter_key_type { none, capture_id, source_id };
using filter_map_type = std::map<filter_key_type, std::string>;
using filter_map_ptr = std::unique_ptr<filter_map_type>;

using serialized_msg = openperf::message::serialized_message;

/*
 * Provide a mechanism for associating type information with
 * expected REST query strings.
 * This is probably overkill for two filter types, but we
 * should have more later.
 */
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

constexpr auto filter_key_names =
    associative_array<std::string_view, filter_key_type>(
        std::pair("capture_id", filter_key_type::capture_id),
        std::pair("source_id", filter_key_type::source_id));

constexpr filter_key_type to_key_type(std::string_view name)
{
    auto cursor = std::begin(filter_key_names),
         end = std::end(filter_key_names);
    while (cursor != end) {
        if (cursor->first == name) return (cursor->second);
        cursor++;
    }

    return (filter_key_type::none);
}

constexpr std::string_view to_key_name(filter_key_type key)
{
    auto cursor = std::begin(filter_key_names),
         end = std::end(filter_key_names);
    while (cursor != end) {
        if (cursor->second == key) return (cursor->first);
        cursor++;
    }

    return ("unknown");
}

struct request_list_captures
{
    filter_map_ptr filter;
};

struct request_create_capture
{
    capture_ptr capture;
};

struct request_delete_captures
{};

struct request_get_capture
{
    std::string id;
};

struct request_delete_capture
{
    std::string id;
};

struct request_start_capture
{
    std::string id;
};

struct request_stop_capture
{
    std::string id;
};

struct request_list_capture_results
{
    filter_map_ptr filter;
};

struct request_delete_capture_results
{};

struct request_get_capture_result
{
    std::string id;
};

struct request_delete_capture_result
{
    std::string id;
};

/**
 * The request_create_capture_transfer message is used to attach a transfer
 * object to the capture results object.  If this message is successful the
 * capture results object will own the transfer object but the transfer object
 * may still be used by the transfer thread.
 *
 * The transfer object should not be deleted until the transfer is completed.
 */
struct request_create_capture_transfer
{
    std::string id;
    transfer_context* transfer;
};

struct request_delete_capture_transfer
{
    std::string id;
};

struct reply_captures
{
    std::vector<capture_ptr> captures;
};

struct reply_capture_results
{
    std::vector<capture_result_ptr> capture_results;
};

using request_msg = std::variant<request_list_captures,
                                 request_create_capture,
                                 request_delete_captures,
                                 request_get_capture,
                                 request_delete_capture,
                                 request_start_capture,
                                 request_stop_capture,
                                 request_list_capture_results,
                                 request_delete_capture_results,
                                 request_get_capture_result,
                                 request_delete_capture_result,
                                 request_create_capture_transfer,
                                 request_delete_capture_transfer>;

struct reply_ok
{};

enum class error_type { NONE = 0, NOT_FOUND, POSIX, ZMQ_ERROR };

struct typed_error
{
    error_type type = error_type::NONE;
    int value = 0;
};

struct reply_error
{
    typed_error info;
};

using reply_msg =
    std::variant<reply_captures, reply_capture_results, reply_ok, reply_error>;

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg);
tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

reply_error to_error(error_type type, int value = 0);

capture_ptr to_swagger(const sink&);
capture_result_ptr to_swagger(const core::uuid& id, const sink_result& result);

/* Validation routines */
bool is_valid(const capture_type&, std::vector<std::string>&);

} // namespace api
} // namespace openperf::packet::capture

namespace swagger::v1::model {
void from_json(const nlohmann::json&, swagger::v1::model::PacketCapture&);
} // namespace swagger::v1::model

#endif
