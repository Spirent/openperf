#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <typeinfo>

#include <arpa/inet.h>

#include "swagger/v1/model/Interface.h"
#include "packetio/generic_interface.h"

namespace icp {
namespace packetio {
namespace interface {

using namespace swagger::v1::model;

/**
 * This struct is magic.  Use templates and parameter packing to provide
 * some syntactic sugar for creating visitor objects for std::visit.
 */
template<typename ...Ts>
struct overloaded_visitor : Ts...
{
    overloaded_visitor(const Ts&... args)
        : Ts(args)...
    {}

    using Ts::operator()...;
};

static void validate(const eth_protocol_config& eth,
                     std::vector<std::string>& errors)
{
    if (eth.address.is_broadcast()) {
        errors.emplace_back("Broadcast MAC address (" + net::to_string(eth.address)
                            + ") is not allowed.");
    } else if (eth.address.is_multicast()) {
        errors.emplace_back("Multicast MAC address (" + net::to_string(eth.address)
                            + ") is not allowed.");
    }
}

static void validate(const ipv4_dhcp_protocol_config&,
                     std::vector<std::string>&)
{
    /* DHCP config is always valid */
}

static void validate(const ipv4_static_protocol_config& ipv4,
                     std::vector<std::string>& errors)
{
    if (ipv4.prefix_length > 32) {
        errors.emplace_back("Prefix length (" + std::to_string(ipv4.prefix_length)
                            + ") is too big.");
    }
    if (ipv4.address.is_loopback()) {
        errors.emplace_back("Cannot use loopback address (" + net::to_string(ipv4.address)
                            + ") for interface.");
    } else if (ipv4.address.is_multicast()) {
        errors.emplace_back("Cannot use multicast address (" + net::to_string(ipv4.address)
                            + ") for interface.");
    }

    /* check optional gateway */
    if (ipv4.gateway) {
        if (ipv4.gateway->is_loopback()) {
            errors.emplace_back("Cannot use loopback address ("
                                + net::to_string(*ipv4.gateway)
                                + ") for gateway.");
        } else if (ipv4.gateway->is_multicast()) {
            errors.emplace_back("Cannot use multicast address ("
                                + net::to_string(*ipv4.gateway)
                                + ") for gateway.");
        } else if (net::ipv4_network(*ipv4.gateway, ipv4.prefix_length) !=
                   net::ipv4_network(ipv4.address, ipv4.prefix_length)) {
            errors.emplace_back("Gateway address (" + net::to_string(*ipv4.gateway)
                                + ") is not in the same broadcast domain as interface ("
                                + net::to_string(net::ipv4_network(ipv4.address,
                                                                   ipv4.prefix_length))
                                + ").");
        }
    }
}

static void validate(const ipv4_protocol_config& ipv4,
                     std::vector<std::string>& errors)
{
    auto visitor = overloaded_visitor(
        [&](const ipv4_dhcp_protocol_config& dhcp_ipv4) {
            validate(dhcp_ipv4, errors);
        },
        [&](const ipv4_static_protocol_config& static_ipv4) {
            validate(static_ipv4, errors);
        });

    std::visit(visitor, ipv4);
}

/**
 * This is a wrapper around a simple std::unordered_map to allow
 * us to count types.
 */
class type_info_counter
{
public:
    using type_info_ref = std::reference_wrapper<const std::type_info>;

    int& operator[](type_info_ref ref) { return m_counts[ref]; }

private:
    struct hasher
    {
        size_t operator()(type_info_ref code) const
        {
            return code.get().hash_code();
        }
    };

    struct equals
    {
        bool operator()(type_info_ref lhs, type_info_ref rhs) const
        {
            return (lhs.get() == rhs.get());
        }
    };

    std::unordered_map<type_info_ref, int, hasher, equals> m_counts;
};

bool is_valid(config_data& config, std::vector<std::string>& errors)
{
    /*
     * We use the protocol counter to tally the protocol types we have in
     * our configuration vector.
     */
    type_info_counter protocol_counter;
    auto protocol_visitor = overloaded_visitor(
        [&](const eth_protocol_config& eth) {
            protocol_counter[typeid(eth)]++;
            validate(eth, errors);
        },
        [&](const ipv4_protocol_config& ipv4) {
            protocol_counter[typeid(ipv4)]++;
            validate(ipv4, errors);
        });

    for (auto& protocol : config.protocols) {
        std::visit(protocol_visitor, protocol);
    }

    /* Currently we require 1 Ethernet protocol config and at most 1 IPv4 protocol config */
    if (auto count = protocol_counter[typeid(eth_protocol_config)]; count != 1) {
        errors.emplace_back("At " + std::string(count == 0 ? "least" : "most") + " one Ethernet "
                            + "protocol configuration is required per interface.");
    }
    if (auto count = protocol_counter[typeid(ipv4_protocol_config)]; count > 1) {
        errors.emplace_back("At most one IPv4 protocol configuration is allowed per interface, "
                            "not " + std::to_string(count) + ".");
    }

    /*
     * XXX: swagger specs says we care about order, but at this point, our configuration
     * is unambiguous, so don't bother.
     */

    return (errors.size() == 0);
}

void from_json(const nlohmann::json& j, eth_protocol_config& config)
{
    config.address = net::mac_address(j["mac_address"].get<std::string>());
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
        config.gateway = std::make_optional(net::ipv4_address(*gw));
    }
    config.address = net::ipv4_address(j["address"].get<std::string>());
    /* XXX: Swagger spec doesn't allow smaller ints? */
    config.prefix_length = (j["prefix_length"].get<int32_t>()
                            & std::numeric_limits<uint8_t>::max());
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

    std::vector<std::string> errors;
    if (!is_valid(config, errors)) {
        throw std::runtime_error(
            std::accumulate(begin(errors), end(errors), std::string(),
                            [](const std::string &a, const std::string &b) -> std::string {
                                return a + (a.length() > 0 ? " " : "") + b;
                            }));
    }
}

static
std::shared_ptr<InterfaceProtocolConfig_eth>
make_swagger_protocol_config(const eth_protocol_config& eth)
{
    auto config = std::make_shared<InterfaceProtocolConfig_eth>();

    config->setMacAddress(to_string(eth.address));

    return (config);
}

static
std::shared_ptr<InterfaceProtocolConfig_ipv4_dhcp>
make_swagger_protocol_config(const ipv4_dhcp_protocol_config& dhcp_ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4_dhcp>();
    if (dhcp_ipv4.hostname) {
        config->setHostname(*dhcp_ipv4.hostname);
    }
    if (dhcp_ipv4.client) {
        config->setClient(*dhcp_ipv4.client);
    }

    return (config);
}

static
std::shared_ptr<InterfaceProtocolConfig_ipv4_static>
make_swagger_protocol_config(const ipv4_static_protocol_config& static_ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4_static>();

    if (static_ipv4.gateway) {
        config->setGateway(to_string(*static_ipv4.gateway));
    }
    config->setAddress(to_string(static_ipv4.address));
    config->setPrefixLength(static_ipv4.prefix_length);

    return (config);
}

static
std::shared_ptr<InterfaceProtocolConfig_ipv4>
make_swagger_protocol_config(const ipv4_protocol_config& ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4>();

    auto visitor = overloaded_visitor(
        [&](const ipv4_dhcp_protocol_config &dhcp_ipv4) {
            config->setDhcp(make_swagger_protocol_config(dhcp_ipv4));
            config->setMethod("static");
        },
        [&](const ipv4_static_protocol_config &static_ipv4) {
            config->setStatic(make_swagger_protocol_config(static_ipv4));
            config->setMethod("static");
        });

    std::visit(visitor, ipv4);

    return (config);
}

static
std::shared_ptr<InterfaceProtocolConfig>
make_swagger_protocol_config(const protocol_config& protocol)
{
    auto config = std::make_shared<InterfaceProtocolConfig>();

    auto visitor = overloaded_visitor(
        [&](const eth_protocol_config& eth) {
            config->setEth(make_swagger_protocol_config(eth));
        },
        [&](const ipv4_protocol_config& ipv4) {
            config->setIpv4(make_swagger_protocol_config(ipv4));
        });

    std::visit(visitor, protocol);

    return (config);
}

static
std::shared_ptr<Interface_config>
make_swagger_interface_config(const config_data& in_config)
{
    auto config = std::make_shared<Interface_config>();
    for (auto& protocol : in_config.protocols) {
        config->getProtocols().emplace_back(make_swagger_protocol_config(protocol));
    }
    return (config);
}

static
std::shared_ptr<InterfaceStats>
make_swagger_interface_stats(const generic_interface& intf)
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
