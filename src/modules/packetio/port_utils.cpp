#include <string>
#include <variant>

#include "json.hpp"

#include "swagger/v1/model/Port.h"
#include "packetio/generic_port.hpp"
#include "utils/overloaded_visitor.hpp"

namespace openperf::packetio::port {

using namespace swagger::v1::model;

std::string to_string(const link_status& status)
{
    switch (status) {
    case link_status::LINK_UP:
        return "up";
    case link_status::LINK_DOWN:
        return "down";
    default:
        return "unknown";
    }
}

std::string to_string(const link_duplex& duplex)
{
    switch (duplex) {
    case link_duplex::DUPLEX_FULL:
        return "full";
    case link_duplex::DUPLEX_HALF:
        return "half";
    default:
        return "unknown";
    }
}

std::string to_string(const link_speed& speed)
{
    return std::to_string(static_cast<int>(speed)) + " Mbps";
}

std::string to_string(const lag_mode& mode)
{
    switch (mode) {
    case lag_mode::LAG_802_3_AD:
        return "lag_802_3_ad";
    default:
        return "unknown";
    }
}

static void validate(const bond_config& bond, std::vector<std::string>& errors)
{
    if (bond.mode != lag_mode::LAG_802_3_AD) {
        errors.emplace_back("Unrecognized LAG mode; only 'lag_802_3_ad' is "
                            "currently supported.");
    }

    if (bond.ports.size() < 2) {
        errors.emplace_back(
            "At least two ports are required for link bonding.");
    }
}

static void validate(const dpdk_config& dpdk, std::vector<std::string>& errors)
{
    /* Only need to validate speed/duplex when auto is disabled */
    if (dpdk.auto_negotiation) { return; }

    if (dpdk.duplex == link_duplex::DUPLEX_UNKNOWN) {
        errors.emplace_back(
            "Unrecognized duplex setting; please only use 'full' or 'half'.");
    }

    switch (dpdk.speed) {
    case link_speed::SPEED_10M:
    case link_speed::SPEED_100M:
        /* Ok with any valid duplex setting */
        break;
    case link_speed::SPEED_1G:
    case link_speed::SPEED_2_5G:
    case link_speed::SPEED_5G:
    case link_speed::SPEED_10G:
    case link_speed::SPEED_20G:
    case link_speed::SPEED_25G:
    case link_speed::SPEED_40G:
    case link_speed::SPEED_50G:
    case link_speed::SPEED_56G:
    case link_speed::SPEED_100G:
        if (dpdk.duplex == link_duplex::DUPLEX_HALF) {
            errors.emplace_back("Half duplex cannot be used with "
                                + to_string(dpdk.speed) + " speeds.");
        }
        break;
    default:
        errors.emplace_back(
            "Unrecognized speed setting: " + to_string(dpdk.speed) + ".");
    }
}

static bool is_valid(config_data& config, std::vector<std::string>& errors)
{
    auto config_visitor = utils::overloaded_visitor(
        [&](const bond_config& bond) { validate(bond, errors); },
        [&](const dpdk_config& dpdk) { validate(dpdk, errors); },
        [&](const std::monostate&) {
            errors.emplace_back("No valid port configuration found.");
        });

    std::visit(config_visitor, config);

    return (errors.size() == 0);
}

static bond_config from_swagger(const std::shared_ptr<PortConfig_bond>& config)
{
    bond_config to_return;

    to_return.mode =
        (config->getMode() == "lag_802_3_ad" ? lag_mode::LAG_802_3_AD
                                             : lag_mode::LAG_UNKNOWN);

    for (auto& id : config->getPorts()) { to_return.ports.push_back(id); }

    return (to_return);
}

static dpdk_config from_swagger(const std::shared_ptr<PortConfig_dpdk>& config)
{
    dpdk_config to_return;
    to_return.auto_negotiation = config->isAutoNegotiation();

    if (config->speedIsSet()) {
        to_return.speed = static_cast<link_speed>(config->getSpeed());
    }

    if (config->duplexIsSet()) {
        auto duplex = config->getDuplex();
        to_return.duplex =
            (duplex == "full" ? link_duplex::DUPLEX_FULL
                              : duplex == "half" ? link_duplex::DUPLEX_HALF
                                                 : link_duplex::DUPLEX_UNKNOWN);
    }

    return (to_return);
}

config_data make_config_data(const Port& port)
{
    config_data to_return;

    auto config = port.getConfig();
    if (config) {
        if (config->bondIsSet()) {
            to_return = from_swagger(config->getBond());
        } else if (config->dpdkIsSet()) {
            to_return = from_swagger(config->getDpdk());
        }
    }

    std::vector<std::string> errors;
    if (!is_valid(to_return, errors)) {
        throw std::runtime_error(std::accumulate(
            begin(errors),
            end(errors),
            std::string(),
            [](const std::string& a, const std::string& b) -> std::string {
                return a + (a.length() > 0 ? " " : "") + b;
            }));
    }

    return (to_return);
}

/**
 * Functions to transmogrify generic_ports to swagger ports
 */

static std::shared_ptr<PortConfig_dpdk>
make_swagger_port_config_dpdk(const generic_port& port)
{
    auto config_dpdk = std::make_shared<PortConfig_dpdk>();
    auto config = std::get<dpdk_config>(port.config());

    config_dpdk->setAutoNegotiation(config.auto_negotiation);

    if (!config.auto_negotiation) {
        config_dpdk->setSpeed(static_cast<int>(config.speed));
        config_dpdk->setDuplex(to_string(config.duplex));
    }

    return (config_dpdk);
}

static std::shared_ptr<PortConfig_bond>
make_swagger_port_config_bond(const generic_port& port)
{
    auto config_bond = std::make_shared<PortConfig_bond>();
    auto config = std::get<bond_config>(port.config());

    config_bond->setMode(to_string(config.mode));

    config_bond->getPorts().insert(config_bond->getPorts().end(),
                                   config.ports.begin(),
                                   config.ports.end());

    return (config_bond);
}

static std::shared_ptr<PortConfig>
make_swagger_port_config(const generic_port& port)
{
    auto config = std::make_shared<PortConfig>();

    if (port.kind() == "dpdk") {
        config->setDpdk(make_swagger_port_config_dpdk(port));
    } else if (port.kind() == "bond") {
        config->setBond(make_swagger_port_config_bond(port));
    }

    return (config);
}

static std::shared_ptr<PortStatus>
make_swagger_port_status(const generic_port& port)
{
    auto status = std::make_shared<PortStatus>();

    status->setLink(to_string(port.link()));
    status->setSpeed(static_cast<int>(port.speed()));
    status->setDuplex(to_string(port.duplex()));

    return (status);
}

static std::shared_ptr<PortStats>
make_swagger_port_stats(const generic_port& port)
{
    auto stats = std::make_shared<PortStats>();
    auto in_stats = port.stats();

    stats->setRxPackets(in_stats.rx_packets);
    stats->setTxPackets(in_stats.tx_packets);
    stats->setRxBytes(in_stats.rx_bytes);
    stats->setTxBytes(in_stats.tx_bytes);
    stats->setRxErrors(in_stats.rx_errors);
    stats->setTxErrors(in_stats.tx_errors);

    return (stats);
}

std::shared_ptr<Port> make_swagger_port(const generic_port& in_port)
{
    auto out_port = std::make_shared<Port>();

    out_port->setId(in_port.id());
    out_port->setKind(in_port.kind());
    out_port->setConfig(make_swagger_port_config(in_port));
    out_port->setStatus(make_swagger_port_status(in_port));
    out_port->setStats(make_swagger_port_stats(in_port));

    return (out_port);
}

} // namespace openperf::packetio::port
