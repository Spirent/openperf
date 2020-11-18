#include "memory.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/memory.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "modules/memory/api.hpp"
#include "modules/memory/api_converters.hpp"

namespace openperf::tvlp::internal::worker {

namespace swagger = swagger::v1::model;
using namespace openperf::memory::api;
// using namespace Pistache;

memory_tvlp_worker_t::memory_tvlp_worker_t(
    void* context, const model::tvlp_profile_t::series& profile)
    : tvlp_worker_t(context, endpoint, profile)
{}

memory_tvlp_worker_t::~memory_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
memory_tvlp_worker_t::send_create(const model::tvlp_profile_t::entry& entry,
                                  double load_scale)
{
    auto config = swagger::MemoryGeneratorConfig{};
    config.fromJson(const_cast<nlohmann::json&>(entry.config));

    // Apply Load Scale to generator configuration
    config.setReadSize(
        static_cast<uint32_t>(config.getReadSize() * load_scale));
    config.setReadsPerSec(
        static_cast<uint32_t>(config.getReadsPerSec() * load_scale));
    config.setWriteSize(
        static_cast<uint32_t>(config.getWriteSize() * load_scale));
    config.setWritesPerSec(
        static_cast<uint32_t>(config.getWritesPerSec() * load_scale));

    request::generator::create data{
        .is_running = false,
        .config = from_swagger(config),
    };

    auto api_reply =
        submit_request(serialize(std::move(data))).and_then(deserialize_reply);

    if (auto r = std::get_if<reply::generator::item>(&api_reply.value())) {
        return r->id;
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        auto e = to_error(*error);
        if (e.second) return tl::make_unexpected(e.second.value());
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<stat_pair_t, std::string>
memory_tvlp_worker_t::send_start(const std::string& id,
                                 const dynamic::configuration& dynamic_results)
{
    auto api_reply = submit_request(serialize(request::generator::start{
                                        .id = id,
                                        .dynamic_results = dynamic_results,
                                    }))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::statistic::item>(&api_reply.value())) {
        return std::pair(r->id, to_swagger(*r).toJson());
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        auto e = to_error(*error);
        if (e.second) return tl::make_unexpected(e.second.value());
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
        auto e = to_error(*error);
        if (e.second) return tl::make_unexpected(e.second.value());
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
memory_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply =
        submit_request(serialize(request::statistic::get{{.id = id}}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply::statistic::item>(&api_reply.value())) {
        return to_swagger(*r).toJson();
    } else if (auto error = std::get_if<reply::error>(&api_reply.value())) {
        auto e = to_error(*error);
        if (e.second) return tl::make_unexpected(e.second.value());
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
        auto e = to_error(*error);
        if (e.second) return tl::make_unexpected(e.second.value());
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker