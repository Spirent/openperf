#include "memory.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/memory.hpp"
#include "swagger/v1/model/MemoryGenerator.h"

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

    auto result = openperf::api::client::internal_api_post(
        m_generator_endpoint, gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto mg = nlohmann::json::parse(result.second).get<MemoryGenerator>();
    return mg.getId();
}

} // namespace openperf::tvlp::internal::worker