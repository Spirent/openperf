#include <string>

#include "json.hpp"

#include "swagger/v1/model/Port.h"
#include "packetio/generic_port.h"

namespace icp {
namespace packetio {
namespace port {

using namespace swagger::v1::model;

std::string to_string(const link_status &status)
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

std::string to_string(const link_speed &speed)
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


void from_json(const nlohmann::json& j, config_data& c)
{
    if (j.find("dpdk") != j.end()) {
        auto jconf = j["dpdk"];
        c = dpdk_config{ .auto_negotiation = jconf["auto_negotiation"].get<bool>(),
                         .speed = (jconf.find("speed") == jconf.end()
                                   ? link_speed::SPEED_UNKNOWN
                                   : jconf["speed"].get<link_speed>()),
                         .duplex = (jconf.find("duplex") == jconf.end() ? link_duplex::DUPLEX_UNKNOWN
                                    : jconf["duplex"].get<std::string>() == "full" ? link_duplex::DUPLEX_FULL
                                    : link_duplex::DUPLEX_HALF)};
    } else if (j.find("bond") != j.end()) {
        auto jconf = j["bond"];
        c = bond_config{ .mode = (jconf["mode"].get<std::string>() == "lag_802_3_ad"
                                  ? lag_mode::LAG_802_3_AD
                                  : lag_mode::LAG_UNKNOWN) };

        assert(jconf["ports"].is_array());
        for (auto& id : jconf["ports"]) {
            std::get<bond_config>(c).ports.push_back(id.get<int>());
        }
    }
}


/**
 * Functions to transmogrify generic_ports to swagger ports
 */

static std::shared_ptr<PortConfig_dpdk> make_swagger_port_config_dpdk(const generic_port& port)
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

static std::shared_ptr<PortConfig_bond> make_swagger_port_config_bond(const generic_port& port)
{
    auto config_bond = std::make_shared<PortConfig_bond>();
    auto config = std::get<bond_config>(port.config());

    config_bond->setMode(to_string(config.mode));
    std::transform(begin(config.ports), end(config.ports), std::back_inserter(config_bond->getPorts()),
                   [](int id) { return std::to_string(id); });

    return (config_bond);
}

static std::shared_ptr<PortConfig> make_swagger_port_config(const generic_port& port)
{
    auto config = std::make_shared<PortConfig>();

    if (port.kind() == "dpdk") {
        config->setDpdk(make_swagger_port_config_dpdk(port));
    } else if (port.kind() == "bond") {
        config->setBond(make_swagger_port_config_bond(port));
    }

    return (config);
}

static std::shared_ptr<PortStatus> make_swagger_port_status(const generic_port& port)
{
    auto status = std::make_shared<PortStatus>();

    status->setLink(to_string(port.link()));
    status->setSpeed(static_cast<int>(port.speed()));
    status->setDuplex(to_string(port.duplex()));

    return (status);
}

static std::shared_ptr<PortStats> make_swagger_port_stats(const generic_port& port)
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

    out_port->setId(std::to_string(in_port.id()));
    out_port->setKind(in_port.kind());
    out_port->setConfig(make_swagger_port_config(in_port));
    out_port->setStatus(make_swagger_port_status(in_port));
    out_port->setStats(make_swagger_port_stats(in_port));

    return (out_port);
}


}
}
}
