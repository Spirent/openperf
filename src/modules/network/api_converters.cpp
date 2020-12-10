#include "api_converters.hpp"

#include <iomanip>
#include <sstream>

#include "swagger/v1/model/BulkCreateNetworkServersRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkServersRequest.h"
#include "swagger/v1/model/BulkCreateNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkDeleteNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStartNetworkGeneratorsRequest.h"
#include "swagger/v1/model/BulkStopNetworkGeneratorsRequest.h"
#include "swagger/v1/model/NetworkGenerator.h"
#include "swagger/v1/model/NetworkGeneratorResult.h"
#include "swagger/v1/model/NetworkServer.h"

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
        return "unknown";
    }
}

constexpr std::optional<model::address_family_t>
address_family_from_string(std::string_view str)
{
    if (str == "inet") return model::address_family_t::INET;
    if (str == "inet6") return model::address_family_t::INET6;
    return std::nullopt;
}

std::string to_string(model::address_family_t address_family)
{
    switch (address_family) {
    case model::address_family_t::INET:
        return "inet";
    case model::address_family_t::INET6:
        return "inet6";
    default:
        return "unknown";
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
    if (server.addressFamilyIsSet()
        && !address_family_from_string(server.getAddressFamily()))
        errors.emplace_back("Address Family value is not valid.");
    if (!errors.empty()) return tl::make_unexpected(std::move(errors));
    return true;
}

model::generator from_swagger(const swagger::NetworkGenerator& generator)
{
    model::generator_config config;
    config.connections = generator.getConfig()->getConnections();
    config.ops_per_connection = generator.getConfig()->getOpsPerConnection();
    config.read_size = generator.getConfig()->getReadSize();
    config.reads_per_sec = generator.getConfig()->getReadsPerSec();
    config.write_size = generator.getConfig()->getWriteSize();
    config.writes_per_sec = generator.getConfig()->getWritesPerSec();

    model::generator_target target;
    target.host = generator.getConfig()->getTarget()->getHost();
    target.port = generator.getConfig()->getTarget()->getPort();
    auto protocol =
        protocol_from_string(generator.getConfig()->getTarget()->getProtocol());
    if (protocol) target.protocol = protocol.value();
    if (generator.getConfig()->getTarget()->interfaceIsSet())
        target.interface = generator.getConfig()->getTarget()->getInterface();

    model::generator gen_model;
    gen_model.id(generator.getId());
    gen_model.running(generator.isRunning());
    gen_model.config(config);
    gen_model.target(target);
    return gen_model;
}

request::generator::bulk::create
from_swagger(swagger::BulkCreateNetworkGeneratorsRequest& p_request)
{
    request::generator::bulk::create request;
    std::transform(std::begin(p_request.getItems()),
                   std::end(p_request.getItems()),
                   std::back_inserter(request.generators),
                   [](const auto& item) {
                       return std::make_unique<model::generator>(
                           from_swagger(*item));
                   });
    return request;
}

request::generator::bulk::erase
from_swagger(swagger::BulkDeleteNetworkGeneratorsRequest& p_request)
{
    request::generator::bulk::erase request{};
    std::copy(p_request.getIds().begin(),
              p_request.getIds().end(),
              back_inserter(request.ids));
    return request;
}

model::server from_swagger(const swagger::NetworkServer& server)
{
    model::server server_model;
    server_model.id(server.getId());
    server_model.port(server.getPort());
    server_model.protocol(protocol_from_string(server.getProtocol())
                              .value_or(model::protocol_t::TCP));
    if (server.interfaceIsSet()) {
        server_model.interface(server.getInterface());
    }
    if (server.addressFamilyIsSet()) {
        server_model.address_family(
            address_family_from_string(server.getAddressFamily())
                .value_or(model::address_family_t::INET6));
    }
    return server_model;
}

request::server::bulk::create
from_swagger(swagger::BulkCreateNetworkServersRequest& p_request)
{
    request::server::bulk::create request;
    std::transform(std::begin(p_request.getItems()),
                   std::end(p_request.getItems()),
                   std::back_inserter(request.servers),
                   [](const auto& item) {
                       return std::make_unique<model::server>(
                           from_swagger(*item));
                   });
    return request;
}

request::server::bulk::erase
from_swagger(swagger::BulkDeleteNetworkServersRequest& p_request)
{
    request::server::bulk::erase request{};
    std::copy(p_request.getIds().begin(),
              p_request.getIds().end(),
              back_inserter(request.ids));
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
    if (model.target().interface)
        network_target->setInterface(model.target().interface.value());
    network_config->setTarget(network_target);

    auto gen = std::make_shared<swagger::NetworkGenerator>();
    gen->setId(model.id());
    gen->setRunning(model.running());
    gen->setConfig(network_config);
    return gen;
}

swagger::NetworkGeneratorStats
to_swagger(const model::generator_result::load_stat_t& stat)
{
    swagger::NetworkGeneratorStats model;
    model.setBytesActual(stat.bytes_actual);
    model.setBytesTarget(stat.bytes_target);
    model.setLatencyTotal(
        std::chrono::duration_cast<std::chrono::nanoseconds>(stat.latency)
            .count());
    model.setOpsActual(stat.ops_actual);
    model.setOpsTarget(stat.ops_target);
    model.setIoErrors(stat.io_errors);

    if (stat.latency_max.has_value()) {
        model.setLatencyMax(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                stat.latency_max.value())
                .count());
    }

    if (stat.latency_min.has_value()) {
        model.setLatencyMin(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                stat.latency_min.value())
                .count());
    }

    return model;
}

swagger::NetworkGeneratorConnectionStats
to_swagger(const model::generator_result::conn_stat_t& stat)
{
    swagger::NetworkGeneratorConnectionStats model;
    model.setAttempted(stat.attempted);
    model.setSuccessful(stat.successful);
    model.setClosed(stat.closed);
    model.setErrors(stat.errors);

    return model;
}

std::shared_ptr<swagger::NetworkGeneratorResult>
to_swagger(const model::generator_result& result)
{
    auto gen = std::make_shared<swagger::NetworkGeneratorResult>();
    gen->setId(result.id());
    gen->setGeneratorId(result.generator_id());
    gen->setActive(result.active());
    gen->setTimestampLast(to_rfc3339(result.timestamp().time_since_epoch()));
    gen->setTimestampFirst(
        to_rfc3339(result.start_timestamp().time_since_epoch()));

    gen->setRead(std::make_shared<swagger::NetworkGeneratorStats>(
        to_swagger(result.read_stats())));
    gen->setWrite(std::make_shared<swagger::NetworkGeneratorStats>(
        to_swagger(result.write_stats())));
    gen->setConnection(
        std::make_shared<swagger::NetworkGeneratorConnectionStats>(
            to_swagger(result.conn_stats())));

    gen->setDynamicResults(std::make_shared<swagger::DynamicResults>(
        dynamic::to_swagger(result.dynamic_results())));

    return gen;
}

std::shared_ptr<swagger::NetworkServer> to_swagger(const model::server& model)
{
    auto mstat = model.stat();
    auto stats = std::make_shared<swagger::NetworkServerStats>();
    stats->setBytesReceived(mstat.bytes_received);
    stats->setBytesSent(mstat.bytes_sent);
    stats->setConnections(mstat.connections);
    stats->setClosed(mstat.closed);
    stats->setErrors(mstat.errors);

    auto server = std::make_shared<swagger::NetworkServer>();
    server->setId(model.id());
    server->setPort(model.port());
    server->setProtocol(to_string(model.protocol()));
    if (model.interface()) server->setInterface(model.interface().value());
    if (model.address_family())
        server->setAddressFamily(to_string(model.address_family().value()));
    server->setStats(stats);
    return server;
}

} // namespace openperf::network::api
