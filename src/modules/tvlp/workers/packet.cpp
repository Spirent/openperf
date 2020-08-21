#include "packet.hpp"
#include "swagger/v1/converters/packet_generator.hpp"
#include "swagger/v1/model/PacketGenerator.h"
#include "swagger/v1/model/PacketGeneratorResult.h"

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
    auto blk_conf = std::make_shared<PacketGeneratorConfig>();
    blk_conf->fromJson(const_cast<nlohmann::json&>(config));
    gen.setConfig(blk_conf);

    auto result = openperf::api::client::internal_api_post(
        "/packet-generators", gen.toJson().dump(), INTERNAL_REQUEST_TIMEOUT);

    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);

    auto pg = nlohmann::json::parse(result.second).get<PacketGenerator>();
    return pg.getId();
}
tl::expected<stat_pair_t, std::string>
packet_tvlp_worker_t::send_start(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/packet-generators/" + id + "/start", "", INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    auto stat = nlohmann::json::parse(result.second);
    return std::pair(stat.get<PacketGeneratorResult>().getId(), stat);
}
tl::expected<void, std::string>
packet_tvlp_worker_t::send_stop(const std::string& id)
{
    auto result = openperf::api::client::internal_api_post(
        "/packet-generators/" + id + "/stop", "", INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}
tl::expected<nlohmann::json, std::string>
packet_tvlp_worker_t::send_stat(const std::string& id)
{
    auto result = openperf::api::client::internal_api_get(
        "/packet-generator-results/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return nlohmann::json::parse(result.second);
}
tl::expected<void, std::string>
packet_tvlp_worker_t::send_delete(const std::string& id)
{
    auto result = openperf::api::client::internal_api_del(
        "/packet-generators/" + id, INTERNAL_REQUEST_TIMEOUT);
    if (result.first < Pistache::Http::Code::Ok
        || result.first >= Pistache::Http::Code::Already_Reported)
        return tl::make_unexpected(result.second);
    return {};
}

} // namespace openperf::tvlp::internal::worker