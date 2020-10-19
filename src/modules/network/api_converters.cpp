#include "api_converters.hpp"

#include <iomanip>
#include <sstream>

namespace openperf::network::api {

template <typename Rep, typename Period>
std::string to_rfc3339(std::chrono::duration<Rep, Period> from)
{
    auto tv = openperf::timesync::to_timeval(from);
    std::stringstream os;
    os << std::put_time(gmtime(&tv.tv_sec), "%FT%T") << "." << std::setfill('0')
       << std::setw(6) << tv.tv_usec << "Z";
    return os.str();
}

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::NetworkGeneratorConfig& config)
{
    auto errors = std::vector<std::string>();

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

model::generator from_swagger(const swagger::NetworkGenerator& generator)
{
    model::generator_config config;

    auto swagger_network_config = generator.getConfig();

    model::generator gen_model;
    gen_model.id(generator.getId());
    gen_model.running(generator.isRunning());
    gen_model.config(config);
    return gen_model;
}

request::generator::bulk::create
from_swagger(swagger::BulkCreateNetworkGeneratorsRequest& p_request)
{
    request::generator::bulk::create request;
    for (const auto& item : p_request.getItems())
        request.generators.emplace_back(
            std::make_unique<model::generator>(from_swagger(*item)));
    return request;
}

request::generator::bulk::erase
from_swagger(swagger::BulkDeleteNetworkGeneratorsRequest& p_request)
{
    request::generator::bulk::erase request{};
    for (auto& id : p_request.getIds()) {
        request.ids.push_back(std::make_unique<std::string>(id));
    }
    return request;
}

std::shared_ptr<swagger::NetworkGenerator>
to_swagger(const model::generator& model)
{
    auto network_config = std::make_shared<swagger::NetworkGeneratorConfig>();

    auto gen = std::make_shared<swagger::NetworkGenerator>();
    gen->setId(model.id());
    gen->setRunning(model.running());
    gen->setConfig(network_config);
    return gen;
}

std::shared_ptr<swagger::NetworkGeneratorResult>
to_swagger(const model::generator_result& result)
{
    // auto stats = result.stats();
    auto network_stats = std::make_shared<swagger::NetworkGeneratorStats>();

    auto gen = std::make_shared<swagger::NetworkGeneratorResult>();
    gen->setId(result.id());
    gen->setGeneratorId(result.generator_id());
    gen->setActive(result.active());
    gen->setTimestampLast(to_rfc3339(result.timestamp().time_since_epoch()));
    gen->setTimestampFirst(
        to_rfc3339(result.start_timestamp().time_since_epoch()));
    gen->setDynamicResults(std::make_shared<swagger::DynamicResults>(
        dynamic::to_swagger(result.dynamic_results())));

    return gen;
}

} // namespace openperf::network::api
