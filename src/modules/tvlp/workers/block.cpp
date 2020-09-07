#include "block.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/block.hpp"
#include "swagger/v1/model/BlockGenerator.h"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;

block_tvlp_worker_t::block_tvlp_worker_t(
    const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(profile){};

tl::expected<std::string, std::string>
block_tvlp_worker_t::send_create(const nlohmann::json& config,
                                 const std::string& resource_id)
{
    BlockGenerator gen;
    gen.setResourceId(resource_id);
    auto blk_conf = std::make_shared<BlockGeneratorConfig>();
    blk_conf->fromJson(const_cast<nlohmann::json&>(config));
    gen.setConfig(blk_conf);

    auto result = openperf::api::client::internal_api_post(
        m_generator_endpoint, gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first != Pistache::Http::Code::Created)
        return tl::make_unexpected(result.second);

    auto bg = nlohmann::json::parse(result.second).get<BlockGenerator>();
    return bg.getId();
}

} // namespace openperf::tvlp::internal::worker