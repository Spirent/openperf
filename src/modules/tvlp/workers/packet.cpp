#include "packet.hpp"
#include "api/api_internal_client.hpp"
#include "swagger/converters/packet_generator.hpp"
#include "swagger/v1/model/PacketGenerator.h"

namespace openperf::tvlp::internal::worker {

using namespace swagger::v1::model;

packet_tvlp_worker_t::packet_tvlp_worker_t(
    const model::tvlp_module_profile_t& profile)
    : tvlp_worker_t(profile){};

tl::expected<std::string, std::string>
packet_tvlp_worker_t::send_create(const nlohmann::json& config,
                                  const std::string& target_id)
{
    PacketGenerator gen;
    gen.setTargetId(target_id);
    auto blk_conf = std::make_shared<PacketGeneratorConfig>(
        nlohmann::json::parse(config.dump()).get<PacketGeneratorConfig>());
    gen.setConfig(blk_conf);
    auto result = openperf::api::client::internal_api_post(
        m_generator_endpoint, gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto pg = nlohmann::json::parse(result.second).get<PacketGenerator>();
    return pg.getId();
}

} // namespace openperf::tvlp::internal::worker