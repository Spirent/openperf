#include "packet.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/packet_generator.hpp"
#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"
#include "swagger/v1/model/TxFlow.h"
#include "modules/packet/generator/api.hpp"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;
using namespace openperf::packet::generator::api;
using namespace Pistache;

packet_tvlp_worker_t::packet_tvlp_worker_t(
    void* context, const model::tvlp_profile_t::series& series)
    : tvlp_worker_t(context, std::string(endpoint), series){};

packet_tvlp_worker_t::~packet_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
packet_tvlp_worker_t::send_create(const model::tvlp_profile_t::entry& entry,
                                  double load_scale)
{
    assert(entry.resource_id.has_value());

    auto config = std::make_shared<PacketGeneratorConfig>(
        nlohmann::json::parse(entry.config.dump())
            .get<PacketGeneratorConfig>());

    auto load = config->getLoad();
    load->setBurstSize(
        static_cast<uint32_t>(load->getBurstSize() * load_scale));

    auto rate = config->getLoad()->getRate();
    rate->setValue(static_cast<uint64_t>(rate->getValue() * load_scale));

    PacketGenerator gen;
    gen.setTargetId(entry.resource_id.value());
    gen.setConfig(config);

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

tl::expected<stat_pair_t, std::string> packet_tvlp_worker_t::send_start(
    const std::string& id, const dynamic::configuration& /* dynamic_results */)
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
        submit_request(serialize_request(request_get_generator_result{id}))
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