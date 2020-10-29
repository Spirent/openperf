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

constexpr std::optional<model::protocol_t>
protocol_from_string(std::string_view str)
{
    if (str == "tcp") return model::protocol_t::TCP;
    if (str == "udp") return model::protocol_t::UDP;
    return std::nullopt;
}

std::string to_string(model::protocol_t protocol)
{
    switch (protocol) {
    case model::protocol_t::UDP:
        return "udp";
    case model::protocol_t::TCP:
        return "tcp";
    default:
        return "";
    }
}

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::NetworkGeneratorConfig& config)
{
    auto errors = std::vector<std::string>();

    if (config.getConnections() < 1)
        errors.emplace_back("Connections value is not valid.");
    if (config.getOpsPerConnection() < 1)
        errors.emplace_back("Opperations per connection value is not valid.");
    if (config.getReadSize() < 1)
        errors.emplace_back("Read Size value is not valid.");
    if (config.getReadsPerSec() < 0)
        errors.emplace_back("Reads Per Sec value is not valid.");
    if (config.getWriteSize() < 1)
        errors.emplace_back("Write Size value is not valid.");
    if (config.getWritesPerSec() < 0)
        errors.emplace_back("Writes Per Sec value is not valid.");

    if (config.getReadsPerSec() < 1 && config.getWritesPerSec() < 1)
        errors.emplace_back("No operations were specified.");

    auto target = config.getTarget();
    if (!target) {
        errors.emplace_back("Network target is required.");
    } else {
        if (target->getHost().size() < 1)
            errors.emplace_back("Target host value is not valid.");
        if (target->getPort() < 0 || target->getPort() > 65535)
            errors.emplace_back("Port value is not valid.");
        if (!protocol_from_string(target->getProtocol()))
            errors.emplace_back("Target protocol value is not valid.");
    }

    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

tl::expected<bool, std::vector<std::string>>
is_valid(const swagger::NetworkServer& server)
{
    auto errors = std::vector<std::string>();

    if (server.getPort() < 0 || server.getPort() > 65535)
        errors.emplace_back("Port value is not valid.");
    if (!protocol_from_string(server.getProtocol()))
        errors.emplace_back("Protocol value is not valid.");

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

model::server from_swagger(const swagger::NetworkServer& server)
{
    model::server server_model;
    server_model.id(server.getId());
    server_model.port(server.getPort());
    server_model.protocol(protocol_from_string(server.getProtocol())
                              .value_or(model::protocol_t::TCP));
    return server_model;
}

request::server::bulk::create
from_swagger(swagger::BulkCreateNetworkServersRequest& p_request)
{
    request::server::bulk::create request;
    for (const auto& item : p_request.getItems())
        request.servers.emplace_back(
            std::make_unique<model::server>(from_swagger(*item)));
    return request;
}

request::server::bulk::erase
from_swagger(swagger::BulkDeleteNetworkServersRequest& p_request)
{
    request::server::bulk::erase request{};
    for (auto& id : p_request.getIds()) {
        request.ids.push_back(std::make_unique<std::string>(id));
    }
    return request;
}

std::shared_ptr<swagger::NetworkGenerator>
to_swagger(const model::generator& model)
{
    auto network_config = std::make_shared<swagger::NetworkGeneratorConfig>();
    network_config->setReadSize(model.config().read_size);
    network_config->setReadsPerSec(model.config().reads_per_sec);
    network_config->setWriteSize(model.config().write_size);
    network_config->setWritesPerSec(model.config().writes_per_sec);
    network_config->setConnections(model.config().connections);
    network_config->setOpsPerConnection(model.config().ops_per_connection);

    auto network_target =
        std::make_shared<swagger::NetworkGeneratorConfig_target>();
    network_target->setHost(model.target().host);
    network_target->setPort(model.target().port);
    network_target->setProtocol(to_string(model.target().protocol));
    network_config->setTarget(network_target);

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

std::shared_ptr<swagger::NetworkServer> to_swagger(const model::server& model)
{
    auto server = std::make_shared<swagger::NetworkServer>();
    server->setId(model.id());
    server->setPort(model.port());
    server->setProtocol(to_string(model.protocol()));
    return server;
}

} // namespace openperf::network::api
