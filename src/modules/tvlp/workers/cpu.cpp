#include "cpu.hpp"
#include "swagger/v1/converters/cpu.hpp"
#include "swagger/v1/model/CpuGenerator.h"
#include "swagger/v1/model/CpuGeneratorResult.h"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;

cpu_tvlp_worker_t::cpu_tvlp_worker_t(
    const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(profile){};

tl::expected<std::string, std::string>
cpu_tvlp_worker_t::send_create(const nlohmann::json& config,
                               const std::string& resource_id)
{
    CpuGenerator gen;
    auto blk_conf = std::make_shared<CpuGeneratorConfig>();
    blk_conf->fromJson(const_cast<nlohmann::json&>(config));
    gen.setConfig(blk_conf);

    auto result = openperf::api::client::internal_api_post(
        "/cpu-generators", gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto cg = nlohmann::json::parse(result.second).get<CpuGenerator>();
    return cg.getId();
}
tl::expected<stat_pair_t, std::string>
cpu_tvlp_worker_t::send_start(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/cpu-generators/" + id + "/start", "", INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    auto stat = nlohmann::json::parse(result.second);
    return std::pair(stat.get<CpuGeneratorResult>().getId(), stat);
}
tl::expected<void, std::string>
cpu_tvlp_worker_t::send_stop(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/cpu-generators/" + id + "/stop", "", INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}
tl::expected<nlohmann::json, std::string>
cpu_tvlp_worker_t::send_stat(const std::string& id)
{
    auto result = openperf::api::client::internal_api_get(
        "/cpu-generator-results/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return nlohmann::json::parse(result.second);
}
tl::expected<void, std::string>
cpu_tvlp_worker_t::send_delete(const std::string& id)
{
    auto result = openperf::api::client::internal_api_del(
        "/cpu-generators/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}

} // namespace openperf::tvlp::internal::worker