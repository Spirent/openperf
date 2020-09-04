#include "cpu.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/cpu.hpp"
#include "swagger/v1/model/CpuGenerator.h"

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
        m_generator_endpoint, gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto cg = nlohmann::json::parse(result.second).get<CpuGenerator>();
    return cg.getId();
}
} // namespace openperf::tvlp::internal::worker