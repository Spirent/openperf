#include "network.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/network.hpp"
#include "swagger/v1/model/NetworkGenerator.h"
#include "modules/network/api.hpp"
#include "modules/network/api_converters.hpp"

namespace openperf::tvlp::internal::worker {

namespace swagger = swagger::v1::model;
using namespace openperf::network::api;
// using namespace Pistache;

network_tvlp_worker_t::network_tvlp_worker_t(
    void* context, const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(context, endpoint, profile)
{}

network_tvlp_worker_t::~network_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
network_tvlp_worker_t::send_create(const model::tvlp_profile_entry_t& entry)
{
    auto config = std::make_shared<swagger::NetworkGeneratorConfig>();
    config->fromJson(const_cast<nlohmann::json&>(entry.config));

    // Apply Load Scale to generator configuration
    config->setReadSize(
        static_cast<uint32_t>(config->getReadSize() * entry.load_scale));
    config->setReadsPerSec(
        static_cast<uint32_t>(config->getReadsPerSec() * entry.load_scale));
    config->setWriteSize(
        static_cast<uint32_t>(config->getWriteSize() * entry.load_scale));
    config->setWritesPerSec(
        static_cast<uint32_t>(config->getWritesPerSec() * entry.load_scale));

    swagger::NetworkGenerator gen;
    gen.setConfig(config);

    auto request = request::generator::create{
        std::make_unique<network_generator_t>(from_swagger(gen))};

    auto api_reply = submit_request(serialize_request(std::move(request)))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::generator::list>(&api_reply.value())) {
        return r->generators.front()->id();
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<stat_pair_t, std::string>
network_tvlp_worker_t::send_start(const std::string& id,
                                  const dynamic::configuration& dynamic_results)
{
    auto api_reply = submit_request(serialize_request(request::generator::start{
                                        .id = id,
                                        .dynamic_results = dynamic_results,
                                    }))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::statistic::list>(&api_reply.value())) {
        return std::pair(r->results.front()->id(),
                         to_swagger(*r->results.front())->toJson());
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
network_tvlp_worker_t::send_stop(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request::generator::stop{{.id = id}}))
            .and_then(deserialize_reply);

    if (std::get_if<reply::ok>(&api_reply.value())) { return {}; }
    if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
network_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request::statistic::get{{.id = id}}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::statistic::list>(&api_reply.value())) {
        return to_swagger(*r->results.front())->toJson();
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
network_tvlp_worker_t::send_delete(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request::generator::erase{{.id = id}}))
            .and_then(deserialize_reply);

    if (std::get_if<reply::ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker