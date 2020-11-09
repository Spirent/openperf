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
    void* context, const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(context, endpoint, profile){};

cpu_tvlp_worker_t::~cpu_tvlp_worker_t() { stop(); }

tl::expected<std::string, std::string>
cpu_tvlp_worker_t::send_create(const model::tvlp_profile_entry_t& entry)
{
    auto config = std::make_shared<swagger::CpuGeneratorConfig>();
    config->fromJson(const_cast<nlohmann::json&>(entry.config));
    if (config->getMethod() == "system") {
        config->getSystem()->setUtilization(
            std::min(config->getSystem()->getUtilization() * entry.load_scale,
                     100.0 * op_get_cpu_count()));
    } else if (config->getMethod() == "cores") {
        for (auto& core : config->getCores()) {
            core->setUtilization(
                std::min(core->getUtilization() * entry.load_scale, 100.0));
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

tl::expected<stat_pair_t, std::string>
cpu_tvlp_worker_t::send_start(const std::string& id)
{
    auto api_reply =
        submit_request(serialize_request(request_cpu_generator_start{.id = id}))
            .and_then(deserialize_reply);

    if (auto r = std::get_if<reply_cpu_generator_results>(&api_reply.value())) {
        return std::pair(r->results.front()->id(),
                         to_swagger(*r->results.front())->toJson());
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