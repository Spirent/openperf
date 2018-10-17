#include <cstdio>
#include <memory>
#include <string>

#include <arpa/inet.h>

#include "swagger/v1/model/Interface.h"
#include "packetio/generic_interface.h"

namespace icp {
namespace packetio {
namespace interface {

using namespace swagger::v1::model;

mac_address::mac_address(const std::string& input)
{
    std::string delimiters("-:.");
    size_t beg = 0, pos = 0, idx = 0;
    while((beg = input.find_first_not_of(delimiters, pos)) != std::string::npos) {
        if (idx == 6) {
            throw std::runtime_error("Too many octets in MAC address " + input);
        }
        pos = input.find_first_of(delimiters, beg + 1);
        auto value = std::strtol(input.substr(beg, pos-beg).c_str(), nullptr, 16);
        if (value < 0 || 0xff < value) {
            throw std::runtime_error("MAC address octet " + input.substr(beg, pos-beg)
                                     + " is not between 0x00 and 0xff");
        }
        octets[idx++] = value;
    }
}

mac_address::mac_address(const uint8_t addr[])
    : octets{addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]}
{}

mac_address::mac_address()
    : octets{0, 0, 0, 0, 0, 0}
{}

std::string to_string(const mac_address& mac)
{
    /* Sometimes, C++ is just too cumbersome for it's own good */
    char buffer[18];
    sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x",
            mac.octets[0], mac.octets[1], mac.octets[2],
            mac.octets[3], mac.octets[4], mac.octets[5]);
    return (std::string(buffer));
}

ipv4_address::ipv4_address(const std::string& input)
{
    if (inet_pton(AF_INET, input.c_str(), &uint32) == 0) {
        throw std::runtime_error("Invalid IPv4 address: " + input);
    }
}

ipv4_address::ipv4_address(uint32_t addr)
    : uint32(addr)
{}

ipv4_address::ipv4_address()
    : uint32(0)
{}

std::string to_string(const ipv4_address& ipv4)
{
    char buffer[INET6_ADDRSTRLEN];
    const char *p = inet_ntop(AF_INET, &ipv4, buffer, INET6_ADDRSTRLEN);
    if (p == NULL) {
        throw std::runtime_error("Invalid IPv4 address structure");
    }
    return (std::string(p));
}

void from_json(const nlohmann::json& j, eth_protocol_config& config)
{
    config.address = mac_address(j["mac_address"].get<std::string>());
}


void from_json(const nlohmann::json& j, ipv4_dhcp_protocol_config& config)
{
    config.hostname = get_optional_key<std::string>(j, "hostname");
    config.client = get_optional_key<std::string>(j, "client");
}

void from_json(const nlohmann::json& j, ipv4_static_protocol_config& config)
{
    auto gw = get_optional_key<std::string>(j, "gateway");
    if (gw) {
        config.gateway = std::make_optional(ipv4_address(*gw));
    }
    config.address = ipv4_address(j["address"].get<std::string>());
    auto length = j["prefix_length"].get<int32_t>();
    if (length < 0 || 32 < length) {
        throw std::runtime_error("Prefix length of " + j["prefix_length"].get<std::string>()
                                 + " is invalid for IPv4");
    }
    config.prefix_length = length;
}

void from_json(const nlohmann::json& j, ipv4_protocol_config& config)
{
    if (j.find("static") != j.end()) {
        config = j["static"].get<ipv4_static_protocol_config>();
    } else if (j.find("dhcp") != j.end()) {
        config = j["dhcp"].get<ipv4_dhcp_protocol_config>();
    }
}

void from_json(const nlohmann::json& j, protocol_config& config)
{
    if (j.find("eth") != j.end()) {
        config = j["eth"].get<eth_protocol_config>();
    } else if (j.find("ipv4") != j.end()) {
        config = j["ipv4"].get<ipv4_protocol_config>();
    }
}

void from_json(const nlohmann::json& j, config_data& config)
{
    for (auto& protocol : j["protocols"]) {
        config.protocols.emplace_back(protocol);
    }
}

static std::shared_ptr<InterfaceProtocolConfig_eth> make_swagger_eth_protocol_config(const eth_protocol_config& eth)
{
    auto config = std::make_shared<InterfaceProtocolConfig_eth>();

    config->setMacAddress(to_string(eth.address));

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv4> make_swagger_ipv4_protocol_config(const ipv4_protocol_config& ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4>();

    if (std::holds_alternative<ipv4_static_protocol_config>(ipv4)) {
        auto static_config = std::make_shared<InterfaceProtocolConfig_ipv4_static>();
        auto& static_ipv4 = std::get<ipv4_static_protocol_config>(ipv4);

        if (static_ipv4.gateway) {
            static_config->setGateway(to_string(*static_ipv4.gateway));
        }
        static_config->setAddress(to_string(static_ipv4.address));
        static_config->setPrefixLength(static_ipv4.prefix_length);

        config->setStatic(static_config);
        config->setMethod("static");
    } else if (std::holds_alternative<ipv4_dhcp_protocol_config>(ipv4)) {
        auto dhcp_config = std::make_shared<InterfaceProtocolConfig_ipv4_dhcp>();
        auto& dhcp_ipv4 = std::get<ipv4_dhcp_protocol_config>(ipv4);

        if (dhcp_ipv4.hostname) {
            dhcp_config->setHostname(*dhcp_ipv4.hostname);
        }
        if (dhcp_ipv4.client) {
            dhcp_config->setClient(*dhcp_ipv4.client);
        }

        config->setDhcp(dhcp_config);
        config->setMethod("dhcp");
    }

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig> make_swagger_protocol_config(const protocol_config& protocol)
{
    auto config = std::make_shared<InterfaceProtocolConfig>();

    if (std::holds_alternative<eth_protocol_config>(protocol)) {
        config->setEth(make_swagger_eth_protocol_config(
                           std::get<eth_protocol_config>(protocol)));
    } else if (std::holds_alternative<ipv4_protocol_config>(protocol)) {
        config->setIpv4(make_swagger_ipv4_protocol_config(
                            std::get<ipv4_protocol_config>(protocol)));
    }

    return (config);
}

static std::shared_ptr<Interface_config> make_swagger_interface_config(const config_data& in_config)
{
    auto config = std::make_shared<Interface_config>();
    for (auto& protocol : in_config.protocols) {
        config->getProtocols().emplace_back(make_swagger_protocol_config(protocol));
    }
    return (config);
}

static std::shared_ptr<InterfaceStats> make_swagger_interface_stats(const generic_interface& intf)
{
    auto stats = std::make_shared<InterfaceStats>();
    auto in_stats = intf.stats();

    stats->setRxPackets(in_stats.rx_packets);
    stats->setTxPackets(in_stats.tx_packets);
    stats->setRxBytes(in_stats.rx_bytes);
    stats->setTxBytes(in_stats.tx_bytes);
    stats->setRxErrors(in_stats.rx_errors);
    stats->setTxErrors(in_stats.tx_errors);

    return (stats);
}

std::shared_ptr<Interface> make_swagger_interface(const generic_interface& in_intf)
{
    auto out_intf = std::make_shared<Interface>();

    out_intf->setId(std::to_string(in_intf.id()));
    out_intf->setPortId(std::to_string(in_intf.port_id()));
    out_intf->setConfig(make_swagger_interface_config(in_intf.config()));
    out_intf->setStats(make_swagger_interface_stats(in_intf));

    return (out_intf);
}

}
}
}
