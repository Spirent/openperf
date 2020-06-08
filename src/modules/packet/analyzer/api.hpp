#ifndef _OP_PACKET_ANALZYER_API_HPP_
#define _OP_PACKET_ANALZYER_API_HPP_

#include <string>
#include <memory>
#include <variant>
#include <vector>

#include <zmq.h>

#include "json.hpp"
#include "tl/expected.hpp"

#include "packet/analyzer/statistics/generic_flow_counters.hpp"
#include "packet/analyzer/statistics/generic_protocol_counters.hpp"

namespace swagger::v1::model {

class PacketAnalyzer;
class PacketAnalyzerResult;
class RxFlow;

} // namespace swagger::v1::model

namespace openperf::packet::analyzer {

class sink;
class sink_result;

namespace api {

inline constexpr std::string_view endpoint =
    "inproc://openperf_packet_analyzer";

/*
 * Provide some sane typedefs for the swagger types we deal with.
 *
 * Note: this API is based around moving pointers between the REST API
 * handler and the server backend that actually manages analyzer resources.
 * Strings and vectors of strings are trivial to serialize and can be handled
 * directly. More complex types are wrapped in a std::unique_ptr so that proper
 * ownership semantics can be enforced when objects are shipped across the
 * channel. The serialization/de-serialization routines will release and reset
 * the smart pointers as appropriate and send the raw pointer across the
 * channel.
 */

using protocol_counters_config =
    openperf::utils::bit_flags<statistics::protocol_flags>;
using flow_counters_config = openperf::utils::bit_flags<statistics::flow_flags>;

using analyzer_type = swagger::v1::model::PacketAnalyzer;
using analyzer_ptr = std::unique_ptr<analyzer_type>;

using analyzer_result_type = swagger::v1::model::PacketAnalyzerResult;
using analyzer_result_ptr = std::unique_ptr<analyzer_result_type>;

using rx_flow_type = swagger::v1::model::RxFlow;
using rx_flow_ptr = std::unique_ptr<rx_flow_type>;

enum class filter_key_type { none, analyzer_id, source_id };
using filter_map_type = std::map<filter_key_type, std::string>;
using filter_map_ptr = std::unique_ptr<filter_map_type>;

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
        std::pair("analyzer_id", filter_key_type::analyzer_id),
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

struct request_list_analyzers
{
    filter_map_ptr filter;
};

struct request_create_analyzer
{
    analyzer_ptr analyzer;
};

struct request_delete_analyzers
{};

struct request_get_analyzer
{
    std::string id;
};

struct request_delete_analyzer
{
    std::string id;
};

struct request_start_analyzer
{
    std::string id;
};

struct request_stop_analyzer
{
    std::string id;
};

struct request_list_analyzer_results
{
    filter_map_ptr filter;
};

struct request_delete_analyzer_results
{};

struct request_get_analyzer_result
{
    std::string id;
};

struct request_delete_analyzer_result
{
    std::string id;
};

struct request_list_rx_flows
{
    filter_map_ptr filter;
};

struct request_get_rx_flow
{
    std::string id;
};

struct reply_analyzers
{
    std::vector<analyzer_ptr> analyzers;
};

struct reply_analyzer_results
{
    std::vector<analyzer_result_ptr> analyzer_results;
};

struct reply_rx_flows
{
    std::vector<rx_flow_ptr> flows;
};

using request_msg = std::variant<request_list_analyzers,
                                 request_create_analyzer,
                                 request_delete_analyzers,
                                 request_get_analyzer,
                                 request_delete_analyzer,
                                 request_start_analyzer,
                                 request_stop_analyzer,
                                 request_list_analyzer_results,
                                 request_delete_analyzer_results,
                                 request_get_analyzer_result,
                                 request_delete_analyzer_result,
                                 request_list_rx_flows,
                                 request_get_rx_flow>;

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

using reply_msg = std::variant<reply_analyzers,
                               reply_analyzer_results,
                               reply_rx_flows,
                               reply_ok,
                               reply_error>;

struct serialized_msg
{
    zmq_msg_t type;
    zmq_msg_t data;
};

serialized_msg serialize_request(request_msg&& request);
serialized_msg serialize_reply(reply_msg&& reply);

tl::expected<request_msg, int> deserialize_request(const serialized_msg& msg);
tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg);

int send_message(void* socket, serialized_msg&& msg);
tl::expected<serialized_msg, int> recv_message(void* socket, int flags = 0);

reply_error to_error(error_type type, int value = 0);

analyzer_ptr to_swagger(const sink&);
analyzer_result_ptr to_swagger(const core::uuid& id, const sink_result& result);
rx_flow_ptr to_swagger(const core::uuid& id,
                       const core::uuid& result_id,
                       const statistics::generic_flow_counters& counters);

core::uuid
rx_flow_id(const core::uuid& result_id,
           uint16_t shard_idx,
           uint32_t rss_hash,
           std::optional<uint32_t> signature_stream_id = std::nullopt);
std::tuple<core::uuid, uint16_t, uint32_t, std::optional<uint32_t>>
rx_flow_tuple(const core::uuid& rx_flow_id);

/* Validation routines */
bool is_valid(const analyzer_type&, std::vector<std::string>&);

} // namespace api
} // namespace openperf::packet::analyzer

namespace swagger::v1::model {
void from_json(const nlohmann::json&, swagger::v1::model::PacketAnalyzer&);
} // namespace swagger::v1::model

#endif
