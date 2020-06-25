#include "core/op_core.h"
#include "message/serialized_message.hpp"
#include "packet/generator/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

serialized_msg serialize_request(request_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_generators& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](request_create_generator& request) {
                     return (message::push(serialized,
                                           std::move(request.generator)));
                 },
                 [&](const request_delete_generators&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_start_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_stop_generator& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_bulk_create_generators& request) {
                     return (message::push(serialized, request.generators));
                 },
                 [&](request_bulk_delete_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_start_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_bulk_stop_generators& request) {
                     return (message::push(serialized, request.ids));
                 },
                 [&](request_toggle_generator& request) {
                     return (message::push(serialized, std::move(request.ids)));
                 },
                 [&](request_list_generator_results& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_delete_generator_results&) {
                     return (message::push(serialized, 0));
                 },
                 [&](const request_get_generator_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](const request_delete_generator_result& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 },
                 [&](request_list_tx_flows& request) {
                     return (
                         message::push(serialized, std::move(request.filter)));
                 },
                 [&](const request_get_tx_flow& request) {
                     return (message::push(
                         serialized, request.id.data(), request.id.length()));
                 }),
             msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

serialized_msg serialize_reply(reply_msg&& msg)
{
    serialized_msg serialized;
    auto error =
        (message::push(serialized, msg.index())
         || std::visit(utils::overloaded_visitor(
                           [&](reply_generators& reply) {
                               return (
                                   message::push(serialized, reply.generators));
                           },
                           [&](reply_generator_results& reply) {
                               return (message::push(serialized,
                                                     reply.generator_results));
                           },
                           [&](reply_tx_flows& reply) {
                               return (message::push(serialized, reply.flows));
                           },
                           [&](const reply_ok&) {
                               return (message::push(serialized, 0));
                           },
                           [&](const reply_error& error) {
                               return (message::push(serialized, error.info));
                           }),
                       msg));
    if (error) { throw std::bad_alloc(); }

    return (serialized);
}

tl::expected<request_msg, int> deserialize_request(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<request_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<request_msg, request_list_generators>(): {
        auto request = request_list_generators{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_generator>(): {
        auto request = request_create_generator{};
        request.generator.reset(message::pop<generator_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generators>():
        return (request_delete_generators{});
    case utils::variant_index<request_msg, request_get_generator>(): {
        return (request_get_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_generator>(): {
        return (request_delete_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_start_generator>(): {
        return (request_start_generator({message::pop_string(msg)}));
    }
    case utils::variant_index<request_msg, request_stop_generator>(): {
        return (request_stop_generator{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_create_generators>(): {
        return (request_bulk_create_generators{
            message::pop_unique_vector<generator_type>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_delete_generators>(): {
        return (request_bulk_delete_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_start_generators>(): {
        return (request_bulk_start_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_bulk_stop_generators>(): {
        return (request_bulk_stop_generators{
            message::pop_unique_vector<std::string>(msg)});
    }
    case utils::variant_index<request_msg, request_toggle_generator>(): {
        auto request = request_toggle_generator{};
        using ptr = decltype(request_toggle_generator::ids)::pointer;
        request.ids.reset(message::pop<ptr>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_list_generator_results>(): {
        auto request = request_list_generator_results{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generator_results>():
        return (request_delete_generator_results{});
    case utils::variant_index<request_msg, request_get_generator_result>(): {
        return (request_get_generator_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_delete_generator_result>(): {
        return (request_delete_generator_result{message::pop_string(msg)});
    }
    case utils::variant_index<request_msg, request_list_tx_flows>(): {
        auto request = request_list_tx_flows{};
        request.filter.reset(message::pop<filter_map_type*>(msg));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_tx_flow>(): {
        return (request_get_tx_flow{message::pop_string(msg)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(serialized_msg&& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = message::pop<index_type>(msg);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_generators>(): {
        return (
            reply_generators{message::pop_unique_vector<generator_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_generator_results>(): {
        return (reply_generator_results{
            message::pop_unique_vector<generator_result_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_tx_flows>(): {
        return (reply_tx_flows{message::pop_unique_vector<tx_flow_type>(msg)});
    }
    case utils::variant_index<reply_msg, reply_ok>():
        return (reply_ok{});
    case utils::variant_index<reply_msg, reply_error>():
        return (reply_error{message::pop<typed_error>(msg)});
    }

    return (tl::make_unexpected(EINVAL));
}

reply_error to_error(error_type type, int value)
{
    return (reply_error{.info = typed_error{.type = type, .value = value}});
}

} // namespace openperf::packet::generator::api

namespace swagger::v1::model {

/* std::string needed by json find/[] functions */
nlohmann::json* find_key(const nlohmann::json& j, const std::string& key)
{
    auto ptr = j.find(key) != j.end() && !j[key].is_null()
                   ? std::addressof(j[key])
                   : nullptr;
    return (const_cast<nlohmann::json*>(ptr));
}

void from_json(const nlohmann::json& j, TrafficLoad& load)
{
    load.fromJson(const_cast<nlohmann::json&>(j));

    if (auto jrate = find_key(j, "rate")) {
        auto rate = std::make_shared<TrafficLoad_rate>();
        rate->fromJson(*jrate);
        load.setRate(rate);
    }
}

void from_json(const nlohmann::json& j, TrafficDefinition& definition)
{
    definition.fromJson(const_cast<nlohmann::json&>(j));

    if (auto jlength = find_key(j, "length")) {
        auto length = std::make_shared<TrafficLength>();
        length->fromJson(*jlength);
        definition.setLength(length);
    }

    if (auto jpacket = find_key(j, "packet")) {
        auto packet = std::make_shared<TrafficPacketTemplate>();
        packet->fromJson(*jpacket);
        definition.setPacket(packet);
    }
}

void from_json(const nlohmann::json& j, PacketGeneratorConfig& config)
{
    if (auto jduration = find_key(j, "duration")) {
        auto duration = std::make_shared<TrafficDuration>();
        duration->fromJson(*jduration);
        config.setDuration(duration);
    }

    if (auto jload = find_key(j, "load")) {
        auto load = std::make_shared<TrafficLoad>();
        *load = jload->get<TrafficLoad>();
        config.setLoad(load);
    }

    if (auto jorder = find_key(j, "order")) {
        config.setOrder(jorder->get<std::string>());
    }

    if (auto jcounters = find_key(j, "protocol_counters")) {
        std::transform(std::begin(*jcounters),
                       std::end(*jcounters),
                       std::back_inserter(config.getProtocolCounters()),
                       [](const auto& j_counter) {
                           return (j_counter.template get<std::string>());
                       });
    }

    std::transform(std::begin(j["traffic"]),
                   std::end(j["traffic"]),
                   std::back_inserter(config.getTraffic()),
                   [](const auto& j_def) {
                       auto def = std::make_shared<TrafficDefinition>();
                       *def = j_def.template get<TrafficDefinition>();
                       return (def);
                   });
}

void from_json(const nlohmann::json& j, PacketGenerator& generator)
{
    /*
     * We can't call generator's fromJson function because the user
     * might not specify some of the values even if they technically
     * are required by our swagger spec.
     */
    if (j.find("id") != j.end() && !j["id"].is_null()) {
        generator.setId(j["id"]);
    }

    if (j.find("target_id") != j.end() && !j["target_id"].is_null()) {
        generator.setTargetId(j["target_id"]);
    }

    if (j.find("active") != j.end() && !j["active"].is_null()) {
        generator.setActive(j["active"]);
    }

    if (j.find("config") != j.end() && !j["config"].is_null()) {
        auto config = std::make_shared<PacketGeneratorConfig>();
        *config = j["config"].get<PacketGeneratorConfig>();
        generator.setConfig(config);
    }
}

} // namespace swagger::v1::model
