#include "api/api_internal_client.hpp"
#include "memory.hpp"
#include "modules/memory/api.hpp"
#include "modules/memory/generator/config.hpp"
#include "swagger/converters/memory.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"

namespace openperf::tvlp::internal::worker {

namespace swagger = swagger::v1::model;
using namespace openperf::memory::api;

memory_tvlp_worker_t::memory_tvlp_worker_t(
    void* context, const model::tvlp_profile_t::series& series)
    : tvlp_worker_t(context, endpoint, series)
{}

memory_tvlp_worker_t::~memory_tvlp_worker_t() { stop(); }

static std::string
get_error_string(const openperf::api::rest::typed_error& info)
{
    auto [code, string] = openperf::api::rest::decode_error(info);
    return (string.value_or("Unknown error"));
}

tl::expected<std::string, std::string>
memory_tvlp_worker_t::send_create(const model::tvlp_profile_t::entry& entry,
                                  double load_scale)
{
    auto config = std::make_shared<swagger::MemoryGeneratorConfig>();
    config->fromJson(const_cast<nlohmann::json&>(entry.config));

    // Apply Load Scale to generator configuration
    config->setReadsPerSec(
        static_cast<int64_t>(config->getReadsPerSec() * load_scale));
    config->setWritesPerSec(
        static_cast<int64_t>(config->getWritesPerSec() * load_scale));

    auto gen = std::make_unique<swagger::MemoryGenerator>();
    gen->setConfig(config);

    auto api_reply =
        submit_request(serialize(request::generator::create{std::move(gen)}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::generators>(&api_reply.value())) {
        return r->generators.front()->getId();
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(get_error_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<memory_tvlp_worker_t::start_result_t, std::string>
memory_tvlp_worker_t::send_start(const std::string& id,
                                 const dynamic::configuration& dynamic_results)
{
    auto api_reply = submit_request(serialize(request::generator::start{
                                        .id = id,
                                        .dynamic_results = dynamic_results,
                                    }))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::results>(&api_reply.value())) {
        const auto& result = r->results.front();
        return start_result_t{.result_id = result->getId(),
                              .statistics = result->toJson()};
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(get_error_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
memory_tvlp_worker_t::send_stop(const std::string& id)
{
    auto api_reply =
        submit_request(serialize(request::generator::stop{{.id = id}}))
            .and_then(deserialize_reply);

    if (std::get_if<reply::ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(get_error_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
memory_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply = submit_request(serialize(request::result::get{{.id = id}}))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::results>(&api_reply.value())) {
        return r->results.front()->toJson();
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(get_error_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
memory_tvlp_worker_t::send_delete(const std::string& id)
{
    auto api_reply =
        submit_request(serialize(request::generator::erase{{.id = id}}))
            .and_then(deserialize_reply);

    if (std::get_if<reply::ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        return tl::make_unexpected(get_error_string(error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker
