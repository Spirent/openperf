#include "core/op_core.h"
#include "packet/generator/api.hpp"
#include "utils/overloaded_visitor.hpp"
#include "utils/variant_index.hpp"

#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"

namespace openperf::packet::generator::api {

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
         || std::visit(
             utils::overloaded_visitor(
                 [&](request_list_generators& request) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(request.filter)));
                 },
                 [&](request_create_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(request.generator)));
                 },
                 [&](const request_delete_generators&) {
                     return (zmq_msg_init(&serialized.data, 0));
                 },
                 [&](const request_get_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](const request_delete_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](request_start_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](const request_stop_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](request_bulk_create_generators& request) {
                     return (
                         zmq_msg_init(&serialized.data, request.generators));
                 },
                 [&](request_bulk_delete_generators& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](request_bulk_start_generators& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](request_bulk_stop_generators& request) {
                     return (zmq_msg_init(&serialized.data, request.ids));
                 },
                 [&](request_toggle_generator& request) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(request.ids)));
                 },
                 [&](request_list_generator_results& request) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(request.filter)));
                 },
                 [&](const request_delete_generator_results&) {
                     return (zmq_msg_init(&serialized.data, 0));
                 },
                 [&](const request_get_generator_result& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](const request_delete_generator_result& request) {
                     return (zmq_msg_init(&serialized.data,
                                          request.id.data(),
                                          request.id.length()));
                 },
                 [&](request_list_tx_flows& request) {
                     return (zmq_msg_init(&serialized.data,
                                          std::move(request.filter)));
                 },
                 [&](const request_get_tx_flow& request) {
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
                 [&](reply_generators& reply) {
                     return (zmq_msg_init(&serialized.data, reply.generators));
                 },
                 [&](reply_generator_results& reply) {
                     return (zmq_msg_init(&serialized.data,
                                          reply.generator_results));
                 },
                 [&](reply_tx_flows& reply) {
                     return (zmq_msg_init(&serialized.data, reply.flows));
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
    case utils::variant_index<request_msg, request_list_generators>(): {
        auto request = request_list_generators{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_create_generator>(): {
        auto request = request_create_generator{};
        request.generator.reset(*zmq_msg_data<generator_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generators>():
        return (request_delete_generators{});
    case utils::variant_index<request_msg, request_get_generator>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_generator>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_start_generator>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_start_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_stop_generator>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_stop_generator{std::move(id)});
    }
    case utils::variant_index<request_msg, request_bulk_create_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_type*);
        auto data = zmq_msg_data<generator_type**>(&msg.data);

        auto request = request_bulk_create_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.generators.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_bulk_delete_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(std::string*);
        auto data = zmq_msg_data<std::string**>(&msg.data);

        auto request = request_bulk_delete_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_bulk_start_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(std::string*);
        auto data = zmq_msg_data<std::string**>(&msg.data);

        auto request = request_bulk_start_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_bulk_stop_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(std::string*);
        auto data = zmq_msg_data<std::string**>(&msg.data);

        auto request = request_bulk_stop_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            request.ids.emplace_back(ptr);
        });
        return (request);
    }
    case utils::variant_index<request_msg, request_toggle_generator>(): {
        auto request = request_toggle_generator{};
        using ptr = decltype(request_toggle_generator::ids)::pointer;
        request.ids.reset(*zmq_msg_data<ptr*>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_list_generator_results>(): {
        auto request = request_list_generator_results{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_delete_generator_results>():
        return (request_delete_generator_results{});
    case utils::variant_index<request_msg, request_get_generator_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_delete_generator_result>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_delete_generator_result{std::move(id)});
    }
    case utils::variant_index<request_msg, request_list_tx_flows>(): {
        auto request = request_list_tx_flows{};
        request.filter.reset(*zmq_msg_data<filter_map_type**>(&msg.data));
        return (request);
    }
    case utils::variant_index<request_msg, request_get_tx_flow>(): {
        auto id = std::string(zmq_msg_data<char*>(&msg.data),
                              zmq_msg_size(&msg.data));
        return (request_get_tx_flow{std::move(id)});
    }
    }

    return (tl::make_unexpected(EINVAL));
}

tl::expected<reply_msg, int> deserialize_reply(const serialized_msg& msg)
{
    using index_type = decltype(std::declval<reply_msg>().index());
    auto idx = zmq_msg_data<index_type>(&msg.type);
    switch (idx) {
    case utils::variant_index<reply_msg, reply_generators>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_type*);
        auto data = zmq_msg_data<generator_type**>(&msg.data);

        auto reply = reply_generators{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.generators.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_generator_results>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(generator_result_type*);
        auto data = zmq_msg_data<generator_result_type**>(&msg.data);

        auto reply = reply_generator_results{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.generator_results.emplace_back(ptr);
        });
        return (reply);
    }
    case utils::variant_index<reply_msg, reply_tx_flows>(): {
        auto size = zmq_msg_size(&msg.data) / sizeof(tx_flow_type*);
        auto data = zmq_msg_data<tx_flow_type**>(&msg.data);

        auto reply = reply_tx_flows{};
        std::for_each(data, data + size, [&](const auto& ptr) {
            reply.flows.emplace_back(ptr);
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
