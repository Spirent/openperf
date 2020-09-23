#include "packet.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/packet_generator.hpp"
#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"
#include "modules/packet/generator/api.hpp"

namespace openperf::packet::generator::api {
const char* to_string(const reply_error& error);
}

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;
using namespace openperf::packet::generator::api;
using namespace Pistache;

packet_tvlp_worker_t::packet_tvlp_worker_t(
    void* context, const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(context, std::string(endpoint), profile){};

tl::expected<std::string, std::string>
packet_tvlp_worker_t::send_create(const nlohmann::json& config,
                                  const std::string& target_id)
{
    PacketGenerator gen;
    gen.setTargetId(target_id);
    auto blk_conf = std::make_shared<PacketGeneratorConfig>(
        nlohmann::json::parse(config.dump()).get<PacketGeneratorConfig>());
    gen.setConfig(blk_conf);

    auto api_request = request_create_generator{
        std::make_unique<PacketGenerator>(std::move(gen))};

    auto api_reply = submit_request(serialize_request(std::move(api_request)))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_generators>(&api_reply.value())) {
        return r->generators.front()->getId();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<stat_pair_t, std::string>
packet_tvlp_worker_t::send_start(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_start_generator{id}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_generator_results>(&api_reply.value())) {
        return std::pair(r->generator_results.front()->getId(),
                         r->generator_results.front()->toJson());
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
packet_tvlp_worker_t::send_stop(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_stop_generator{id}))
            .and_then(deserialize_reply);

    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
packet_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_start_generator{id}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_generator_results>(&api_reply.value())) {
        return r->generator_results.front()->toJson();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
packet_tvlp_worker_t::send_delete(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_delete_generator{id}))
            .and_then(deserialize_reply);

    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error));
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker