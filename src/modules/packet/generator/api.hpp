#ifndef _OP_PACKET_GENERATOR_API_HPP_
#define _OP_PACKET_GENERATOR_API_HPP_

#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <zmq.h>

#include "json.hpp"
#include "tl/expected.hpp"

#include "core/op_uuid.hpp"
#include "packet/generator/traffic/sequence.hpp"
#include "units/rate.hpp"

namespace swagger::v1::model {

class PacketGenerator;
class PacketGeneratorResult;
class PacketGeneratorConfig;
class SpirentSignature;
class TrafficDefinition;
class TrafficDuration;
class TrafficLength;
class TrafficLoad;
class TxFlow;

} // namespace swagger::v1::model

namespace openperf::packet::generator {

class source;
struct source_load;
struct source_result;

namespace traffic {
struct counter;
}

namespace api {

inline constexpr std::string_view endpoint =
    "inproc://openperf_packet_generator";

/**
 * Provide some sane typedefs for the swagger types we deal with.
 */

using generator_type = swagger::v1::model::PacketGenerator;
using generator_ptr = std::unique_ptr<generator_type>;

using generator_result_type = swagger::v1::model::PacketGeneratorResult;
using generator_result_ptr = std::unique_ptr<generator_result_type>;

/* Use packets/hour to match what packetio expects */
using tx_rate = openperf::units::rate<uint64_t, std::ratio<1, 3600>>;

using tx_flow_type = swagger::v1::model::TxFlow;
using tx_flow_ptr = std::unique_ptr<tx_flow_type>;

enum class filter_type { none, generator_id, target_id };
using filter_map_type = std::map<filter_type, std::string>;
using filter_map_ptr = std::unique_ptr<filter_map_type>;

/* XXX: What should the min/max be? */
inline constexpr uint16_t min_packet_length = 64;
inline constexpr uint16_t max_packet_length = 9 * 1024;

/*
 * Provide some mappings between REST API strings and strict
 * types. Since each set of strings is relatively small, we
 * just use a static array of pairs instead of embracing the
 * glory and overhead of static maps.
 */
template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

constexpr auto filter_type_names =
    associative_array<std::string_view, filter_type>(
        std::pair("generator_id", filter_type::generator_id),
        std::pair("target_id", filter_type::target_id));

constexpr filter_type to_filter_type(std::string_view name)
{
    auto cursor = std::begin(filter_type_names),
         end = std::end(filter_type_names);
    while (cursor != end) {
        if (cursor->first == name) return (cursor->second);
        cursor++;
    }

    return (filter_type::none);
}

constexpr std::string_view to_filter_name(filter_type key)
{
    auto cursor = std::begin(filter_type_names),
         end = std::end(filter_type_names);
    while (cursor != end) {
        if (cursor->second == key) return (cursor->first);
        cursor++;
    }

    return ("unknown");
}

enum class duration_type { none = 0, indefinite, frames, seconds };
std::string to_duration_string(enum duration_type);

enum class order_type { none = 0, round_robin, sequential };
order_type to_order_type(std::string_view name);

enum class mux_type { none = 0, zip, cartesian };
mux_type to_mux_type(std::string_view name);

enum class load_type { none = 0, frames, octets };
load_type to_load_type(std::string_view name);

enum class period_type { none = 0, hours, milliseconds, minutes, seconds };
period_type to_period_type(std::string_view name);

enum class signature_latency_type { none = 0, start_of_frame, end_of_frame };
signature_latency_type to_signature_latency_type(std::string_view name);

struct request_list_generators
{
    filter_map_ptr filter;
};

struct request_create_generator
{
    generator_ptr generator;
};

struct request_delete_generators
{};

struct request_get_generator
{
    std::string id;
};

struct request_delete_generator
{
    std::string id;
};

struct request_start_generator
{
    std::string id;
};

struct request_stop_generator
{
    std::string id;
};

struct request_list_generator_results
{
    filter_map_ptr filter;
};

struct request_delete_generator_results
{};

struct request_get_generator_result
{
    std::string id;
};

struct request_delete_generator_result
{
    std::string id;
};

struct request_list_tx_flows
{
    filter_map_ptr filter;
};

struct request_get_tx_flow
{
    std::string id;
};

struct reply_generators
{
    std::vector<generator_ptr> generators;
};

struct reply_generator_results
{
    std::vector<generator_result_ptr> generator_results;
};

struct reply_tx_flows
{
    std::vector<tx_flow_ptr> flows;
};

using request_msg = std::variant<request_list_generators,
                                 request_create_generator,
                                 request_delete_generators,
                                 request_get_generator,
                                 request_delete_generator,
                                 request_start_generator,
                                 request_stop_generator,
                                 request_list_generator_results,
                                 request_delete_generator_results,
                                 request_get_generator_result,
                                 request_delete_generator_result,
                                 request_list_tx_flows,
                                 request_get_tx_flow>;

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

using reply_msg = std::variant<reply_generators,
                               reply_generator_results,
                               reply_tx_flows,
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

generator_ptr to_swagger(const source&);

generator_result_ptr to_swagger(const core::uuid& id,
                                const source_result& result);

tx_flow_ptr to_swagger(const core::uuid& id,
                       const core::uuid& result_id,
                       const traffic::counter& counter);

core::uuid tx_flow_id(const core::uuid&, size_t);
std::pair<core::uuid, size_t> tx_flow_tuple(const core::uuid&);

traffic::length_template
to_length_template(const swagger::v1::model::TrafficLength&);

traffic::packet_template
to_packet_template(const swagger::v1::model::TrafficDefinition&);

traffic::signature_config
to_signature_config(const swagger::v1::model::SpirentSignature&);

traffic::sequence to_sequence(const swagger::v1::model::PacketGeneratorConfig&);

source_load to_load(const swagger::v1::model::TrafficLoad&,
                    const traffic::sequence&);

std::optional<size_t>
max_transmit_count(const swagger::v1::model::TrafficDuration&,
                   const source_load&);

/* Validation routines */
bool is_valid(const generator_type&, std::vector<std::string>&);

} // namespace api
} // namespace openperf::packet::generator

namespace swagger::v1::model {
void from_json(const nlohmann::json&, swagger::v1::model::PacketGenerator&);
}

#endif /* _OP_PACKET_GENERATOR_API_HPP_ */
