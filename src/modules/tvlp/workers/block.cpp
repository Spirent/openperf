#include "block.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/block.hpp"
#include "swagger/v1/model/BlockGenerator.h"
#include "modules/block/api.hpp"
#include "modules/block/api_converters.hpp"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;
using namespace openperf::block::api;

block_tvlp_worker_t::block_tvlp_worker_t(
    void* context, const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(context, endpoint, profile)
{}

tl::expected<std::string, std::string>
block_tvlp_worker_t::send_create(const nlohmann::json& config,
                                 const std::string& resource_id)
{
    BlockGenerator gen;
    gen.setResourceId(resource_id);
    auto blk_conf = std::make_shared<BlockGeneratorConfig>();
    blk_conf->fromJson(const_cast<nlohmann::json&>(config));
    gen.setConfig(blk_conf);

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
block_tvlp_worker_t::send_start(const std::string& id)
{
    auto api_reply =
        submit_request(
            serialize_request(request_block_generator_start{.id = id}))
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