#include "block.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/block.hpp"
#include "swagger/v1/model/BlockGenerator.h"
#include "modules/block/api.hpp"
#include "modules/block/api_converters.hpp"

namespace openperf::tvlp::internal::worker {

namespace swagger = swagger::v1::model;
using namespace openperf::block::api;

block_tvlp_worker_t::block_tvlp_worker_t(
    void* context, const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(context, endpoint, profile)
{}

block_tvlp_worker_t::~block_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
block_tvlp_worker_t::send_create(const model::tvlp_profile_entry_t& entry)
{
    assert(entry.resource_id.has_value());

    auto config = std::make_shared<swagger::BlockGeneratorConfig>();
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

    swagger::BlockGenerator gen;
    gen.setResourceId(entry.resource_id.value());
    gen.setConfig(config);

    auto api_reply =
        submit_request(
            serialize_request(request_block_generator_add{
                std::make_unique<openperf::block::model::block_generator>(
                    from_swagger(gen))}))
            .and_then(deserialize_reply);

    if (!api_reply) return tl::make_unexpected(zmq_strerror(api_reply.error()));

    if (auto r = std::get_if<reply_block_generators>(&api_reply.value())) {
        return r->generators.front()->id();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<stat_pair_t, std::string>
block_tvlp_worker_t::send_start(const std::string& id,
                                const dynamic::configuration& dynamic_results)
{
    auto api_reply =
        submit_request(serialize_request(request_block_generator_start{
                           .id = id,
                           .dynamic_results = dynamic_results,
                       }))
            .and_then(deserialize_reply);

    if (auto r =
            std::get_if<reply_block_generator_results>(&api_reply.value())) {
        return std::pair(r->results.front()->id(),
                         to_swagger(*r->results.front())->toJson());
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
block_tvlp_worker_t::send_stop(const std::string& id)
{
    auto api_reply = submit_request(serialize_request(
                                        request_block_generator_stop{.id = id}))
                         .and_then(deserialize_reply);
    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
block_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply =
        submit_request(
            serialize_request(request_block_generator_result{.id = id}))
            .and_then(deserialize_reply);
    if (auto r =
            std::get_if<reply_block_generator_results>(&api_reply.value())) {
        return to_swagger(*r->results.front())->toJson();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
block_tvlp_worker_t::send_delete(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_block_generator_del{.id = id}))
            .and_then(deserialize_reply);

    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker