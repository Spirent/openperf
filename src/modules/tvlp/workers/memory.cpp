#include "memory.hpp"
#include "swagger/v1/converters/memory.hpp"
#include "swagger/v1/model/MemoryGenerator.h"
#include "swagger/v1/model/MemoryGeneratorResult.h"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;

memory_tvlp_worker_t::memory_tvlp_worker_t(
    const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(profile){};

tl::expected<std::string, std::string>
memory_tvlp_worker_t::send_create(const nlohmann::json& config,
                                  const std::string&)
{
    MemoryGenerator gen;
    auto blk_conf = std::make_shared<MemoryGeneratorConfig>();
    blk_conf->fromJson(const_cast<nlohmann::json&>(config));
    gen.setConfig(blk_conf);

    auto result = openperf::api::client::internal_api_post("/memory-generators",
                                                           gen.toJson().dump());

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto mg = nlohmann::json::parse(result.second).get<MemoryGenerator>();
    return mg.getId();
}
tl::expected<stat_pair_t, std::string>
memory_tvlp_worker_t::send_start(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/memory-generators/" + id + "/start", "");
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    auto stat = nlohmann::json::parse(result.second);
    return std::pair(stat.get<MemoryGeneratorResult>().getId(), stat);
}
tl::expected<void, std::string>
memory_tvlp_worker_t::send_stop(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/memory-generators/" + id + "/stop", "");
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}
tl::expected<nlohmann::json, std::string>
memory_tvlp_worker_t::send_stat(const std::string& id)
{
    auto result = openperf::api::client::internal_api_get(
        "/memory-generator-results/" + id);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return nlohmann::json::parse(result.second);
}
tl::expected<void, std::string>
memory_tvlp_worker_t::send_delete(const std::string& id)
{
    auto result =
        openperf::api::client::internal_api_del("/memory-generators/" + id);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}

} // namespace openperf::tvlp::internal::worker