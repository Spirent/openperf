#include "cpu.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/cpu.hpp"
#include "swagger/v1/model/CpuGenerator.h"
#include "modules/cpu/api.hpp"
#include "modules/cpu/api_converters.hpp"

namespace openperf::tvlp::internal::worker {

namespace swagger = swagger::v1::model;
using namespace openperf::cpu::api;

cpu_tvlp_worker_t::cpu_tvlp_worker_t(
    void* context, const model::tvlp_profile_t::series& series)
    : tvlp_worker_t(context, endpoint, series){};

cpu_tvlp_worker_t::~cpu_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
cpu_tvlp_worker_t::send_create(const model::tvlp_profile_t::entry& entry,
                               double load_scale)
{
    auto config = std::make_shared<swagger::CpuGeneratorConfig>();
    config->fromJson(const_cast<nlohmann::json&>(entry.config));
    if (config->getMethod() == "system") {
        config->getSystem()->setUtilization(
            std::min(config->getSystem()->getUtilization() * load_scale,
                     100.0 * op_cpu_count()));
    } else if (config->getMethod() == "cores") {
        for (auto& core : config->getCores()) {
            core->setUtilization(
                std::min(core->getUtilization() * load_scale, 100.0));
        }
    }

    swagger::CpuGenerator gen;
    gen.setConfig(config);

    auto api_request = request_cpu_generator_add{
        std::make_unique<cpu_generator_t>(from_swagger(gen))};
    auto api_reply = submit_request(serialize_request(std::move(api_request)))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_cpu_generators>(&api_reply.value())) {
        return r->generators.front()->id();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<cpu_tvlp_worker_t::start_result_t, std::string>
cpu_tvlp_worker_t::send_start(const std::string& id,
                              const dynamic::configuration& dynamic_results)
{
    auto api_reply =
        submit_request(serialize_request(request_cpu_generator_start{
                           .id = id,
                           .dynamic_results = dynamic_results,
                       }))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_cpu_generator_results>(&api_reply.value())) {
        auto& result = r->results.front();
        return start_result_t{
            .result_id = result->id(),
            .statistics = to_swagger(*result)->toJson(),
            .start_time = result->start_timestamp(),
        };
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
cpu_tvlp_worker_t::send_stop(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_cpu_generator_stop{.id = id}))
            .and_then(deserialize_reply);

    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<nlohmann::json, std::string>
cpu_tvlp_worker_t::send_stat(const std::string& id)
{
    auto api_reply = submit_request(serialize_request(
                                        request_cpu_generator_result{.id = id}))
                         .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_cpu_generator_results>(&api_reply.value())) {
        return to_swagger(*r->results.front())->toJson();
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

tl::expected<void, std::string>
cpu_tvlp_worker_t::send_delete(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_cpu_generator_del{.id = id}))
            .and_then(deserialize_reply);

    if (std::get_if<reply_ok>(&api_reply.value())) {
        return {};
    } else if (auto error = std::get_if<reply_error>(&api_reply.value())) {
        return tl::make_unexpected(to_string(*error->info));
    }

    return tl::make_unexpected("Unexpected error");
}

} // namespace openperf::tvlp::internal::worker