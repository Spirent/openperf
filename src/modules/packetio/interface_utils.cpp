#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <typeinfo>

#include <arpa/inet.h>

#include "swagger/v1/model/Interface.h"
#include "packetio/generic_interface.hpp"
#include "utils/overloaded_visitor.hpp"
#include "core/op_log.h"

namespace openperf {
namespace packetio {
namespace interface {

using namespace swagger::v1::model;

static void validate(const eth_protocol_config& eth,
                     std::vector<std::string>& errors)
{
    if (eth.address.is_broadcast()) {
        errors.emplace_back("Broadcast MAC address ("
                            + net::to_string(eth.address)
                            + ") is not allowed.");
    } else if (eth.address.is_multicast()) {
        errors.emplace_back("Multicast MAC address ("
                            + net::to_string(eth.address)
                            + ") is not allowed.");
    }
}

static void validate(const ipv4_auto_protocol_config&,
                     std::vector<std::string>&)
{
    /* Auto config is always valid */
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
        errors.emplace_back("Prefix length ("
                            + std::to_string(ipv4.prefix_length)
                            + ") is too big.");
    }
    if (ipv4.address.is_loopback()) {
        errors.emplace_back("Cannot use loopback address ("
                            + net::to_string(ipv4.address)
                            + ") for interface.");
    } else if (ipv4.address.is_multicast()) {
        errors.emplace_back("Cannot use multicast address ("
                            + net::to_string(ipv4.address)
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
        } else if (net::ipv4_network(*ipv4.gateway, ipv4.prefix_length)
                   != net::ipv4_network(ipv4.address, ipv4.prefix_length)) {
            errors.emplace_back(
                "Gateway address (" + net::to_string(*ipv4.gateway)
                + ") is not in the same broadcast domain as interface ("
                + net::to_string(
                    net::ipv4_network(ipv4.address, ipv4.prefix_length))
                + ").");
        }
    }
}

static void validate(const ipv4_protocol_config& ipv4,
                     std::vector<std::string>& errors)
{
    auto visitor = utils::overloaded_visitor(
        [&](const ipv4_auto_protocol_config& auto_ipv4) {
            validate(auto_ipv4, errors);
        },
        [&](const ipv4_dhcp_protocol_config& dhcp_ipv4) {
            validate(dhcp_ipv4, errors);
        },
        [&](const ipv4_static_protocol_config& static_ipv4) {
            validate(static_ipv4, errors);
        });

    std::visit(visitor, ipv4);
}

static void validate_ipv6_common(const ipv6_common_protocol_config& config,
                                 std::vector<std::string>& errors)
{
    /* check optional link local address */
    if (config.link_local_address) {
        if (!config.link_local_address->is_linklocal()) {
            errors.emplace_back("Cannot use non link local address ("
                                + net::to_string(*config.link_local_address)
                                + ") for link local address.");
        }
    }
}
static void validate(const ipv6_auto_protocol_config& config,
                     std::vector<std::string>& errors)
{
    validate_ipv6_common(config, errors);
}

static void validate(const ipv6_dhcp6_protocol_config& config,
                     std::vector<std::string>& errors)
{
    validate_ipv6_common(config, errors);
}

static void validate(const ipv6_static_protocol_config& config,
                     std::vector<std::string>& errors)
{
    validate_ipv6_common(config, errors);

    if (config.prefix_length > 128) {
        errors.emplace_back("Prefix length ("
                            + std::to_string(config.prefix_length)
                            + ") is too big.");
    }
    if (config.address.is_loopback()) {
        errors.emplace_back("Cannot use loopback address ("
                            + net::to_string(config.address)
                            + ") for interface.");
    } else if (config.address.is_multicast()) {
        errors.emplace_back("Cannot use multicast address ("
                            + net::to_string(config.address)
                            + ") for interface.");
    }

    /* check optional gateway */
    if (config.gateway) {
        if (config.gateway->is_loopback()) {
            errors.emplace_back("Cannot use loopback address ("
                                + net::to_string(*config.gateway)
                                + ") for gateway.");
        } else if (config.gateway->is_multicast()) {
            errors.emplace_back("Cannot use multicast address ("
                                + net::to_string(*config.gateway)
                                + ") for gateway.");
        } else if (net::ipv6_network(*config.gateway, config.prefix_length)
                   != net::ipv6_network(config.address, config.prefix_length)) {
            errors.emplace_back(
                "Gateway address (" + net::to_string(*config.gateway)
                + ") is not in the same broadcast domain as interface ("
                + net::to_string(
                    net::ipv6_network(config.address, config.prefix_length))
                + ").");
        }
    }
}

static void validate(const ipv6_protocol_config& ipv6,
                     std::vector<std::string>& errors)
{
    auto visitor = utils::overloaded_visitor(
        [&](const ipv6_auto_protocol_config& auto_ipv6) {
            validate(auto_ipv6, errors);
        },
        [&](const ipv6_dhcp6_protocol_config& dhcp_ipv6) {
            validate(dhcp_ipv6, errors);
        },
        [&](const ipv6_static_protocol_config& static_ipv6) {
            validate(static_ipv6, errors);
        });

    std::visit(visitor, ipv6);
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

static bool is_valid(config_data& config, std::vector<std::string>& errors)
{
    /*
     * We use the protocol counter to tally the protocol types we have in
     * our configuration vector.
     */
    type_info_counter protocol_counter;
    auto protocol_visitor = utils::overloaded_visitor(
        [&](const eth_protocol_config& eth) {
            protocol_counter[typeid(eth)]++;
            validate(eth, errors);
        },
        [&](const ipv4_protocol_config& ipv4) {
            protocol_counter[typeid(ipv4)]++;
            validate(ipv4, errors);
        },
        [&](const ipv6_protocol_config& ipv6) {
            protocol_counter[typeid(ipv6)]++;
            validate(ipv6, errors);
        },
        [&](const std::monostate&) {
            errors.emplace_back(
                "No valid interface protocol configuration found.");
        });

    for (auto& protocol : config.protocols) {
        std::visit(protocol_visitor, protocol);
    }

    /* Currently we require 1 Ethernet protocol config and at most 1 IPv4
     * protocol config */
    if (auto count = protocol_counter[typeid(eth_protocol_config)];
        count != 1) {
        errors.emplace_back(
            "At " + std::string(count == 0 ? "least" : "most")
            + " one Ethernet "
            + "protocol configuration is required per interface.");
    }
    if (auto count = protocol_counter[typeid(ipv4_protocol_config)];
        count > 1) {
        errors.emplace_back(
            "At most one IPv4 protocol configuration is allowed per interface, "
            "not "
            + std::to_string(count) + ".");
    }
    if (auto count = protocol_counter[typeid(ipv6_protocol_config)];
        count > 1) {
        errors.emplace_back(
            "At most one IPv6 protocol configuration is allowed per interface, "
            "not "
            + std::to_string(count) + ".");
    }

    /*
     * XXX: swagger specs says we care about order, but at this point, our
     * configuration is unambiguous, so don't bother.
     */

    return (errors.size() == 0);
}

static eth_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_eth>& config)
{
    return (eth_protocol_config{.address =
                                    net::mac_address(config->getMacAddress())});
}

static ipv4_dhcp_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv4_dhcp>& config)
{
    ipv4_dhcp_protocol_config to_return;

    if (config->hostnameIsSet()) {
        to_return.hostname = std::make_optional(config->getHostname());
    }

    if (config->clientIsSet()) {
        to_return.client = std::make_optional(config->getClient());
    }

    return (to_return);
}

static ipv4_static_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv4_static>& config)
{
    ipv4_static_protocol_config to_return;

    if (config->gatewayIsSet()) {
        to_return.gateway =
            std::make_optional(net::ipv4_address(config->getGateway()));
    }

    to_return.address = net::ipv4_address(config->getAddress());
    /* XXX: Can't use small ints in swagger, so cut this value down. */
    to_return.prefix_length =
        config->getPrefixLength() & std::numeric_limits<uint8_t>::max();

    return (to_return);
}

static ipv4_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv4>& config)
{
    ipv4_protocol_config to_return;

    if (config->getMethod() == "auto") {
        to_return = ipv4_auto_protocol_config{};
    } else if (config->getMethod() == "dhcp") {
        if (config->dhcpIsSet()) {
            to_return = from_swagger(config->getDhcp());
        } else {
            to_return = ipv4_dhcp_protocol_config{};
        }
    } else if (config->getMethod() == "static") {
        if (!config->staticIsSet()) {
            OP_LOG(OP_LOG_ERROR,
                   "IPv4 interface config method was static and no static "
                   "config was provided\n");
            return to_return;
        }
        to_return = from_swagger(config->getStatic());
    }

    return (to_return);
}

static ipv6_dhcp6_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv6_dhcp6>& config)
{
    ipv6_dhcp6_protocol_config to_return;

    to_return.stateless = config->isStateless();

    return (to_return);
}

static ipv6_static_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv6_static>& config)
{
    ipv6_static_protocol_config to_return;

    if (config->gatewayIsSet()) {
        to_return.gateway =
            std::make_optional(net::ipv6_address(config->getGateway()));
    }

    to_return.address = net::ipv6_address(config->getAddress());
    /* XXX: Can't use small ints in swagger, so cut this value down. */
    to_return.prefix_length =
        config->getPrefixLength() & std::numeric_limits<uint8_t>::max();

    return (to_return);
}

static ipv6_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv6>& config)
{
    ipv6_protocol_config to_return;
    std::optional<net::ipv6_address> link_local_address;

    if (config->linkLocalAddressIsSet()) {
        link_local_address = std::make_optional(
            net::ipv6_address(config->getLinkLocalAddress()));
    }

    if (config->getMethod() == "auto") {
        ipv6_auto_protocol_config protocol_config;
        protocol_config.link_local_address = link_local_address;
        to_return = protocol_config;
    } else if (config->getMethod() == "dhcp6") {
        if (config->dhcp6IsSet()) {
            auto protocol_config = from_swagger(config->getDhcp6());
            protocol_config.link_local_address = link_local_address;
            to_return = protocol_config;
        } else {
            auto protocol_config =
                ipv6_dhcp6_protocol_config{.stateless = true};
            protocol_config.link_local_address = link_local_address;
            to_return = protocol_config;
        }
    } else if (config->getMethod() == "static") {
        if (!config->staticIsSet()) {
            OP_LOG(OP_LOG_ERROR,
                   "IPv6 interface config method was static and no static "
                   "config was provided\n");
            return to_return;
        }
        auto protocol_config = from_swagger(config->getStatic());
        protocol_config.link_local_address = link_local_address;
        to_return = protocol_config;
    }

    return (to_return);
}

static protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig>& config)
{
    protocol_config to_return;

    if (config->ethIsSet()) {
        to_return = from_swagger(config->getEth());
    } else if (config->ipv4IsSet()) {
        to_return = from_swagger(config->getIpv4());
    } else if (config->ipv6IsSet()) {
        to_return = from_swagger(config->getIpv6());
    }

    return (to_return);
}

config_data make_config_data(const Interface& interface)
{
    config_data to_return;

    auto config = interface.getConfig();
    if (config) {
        for (auto& protocol : config->getProtocols()) {
            to_return.protocols.emplace_back(from_swagger(protocol));
        }
    }

    /* Set id field to what came in from the REST API */
    to_return.id = interface.getId();

    to_return.port_id = interface.getPortId();

    std::vector<std::string> errors;
    /* Now check the actual configuration data */
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

static std::shared_ptr<InterfaceProtocolConfig_eth>
make_swagger_protocol_config(const eth_protocol_config& eth)
{
    auto config = std::make_shared<InterfaceProtocolConfig_eth>();

    config->setMacAddress(to_string(eth.address));

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv4_dhcp>
make_swagger_protocol_config(const ipv4_dhcp_protocol_config& dhcp_ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4_dhcp>();
    if (dhcp_ipv4.hostname) { config->setHostname(*dhcp_ipv4.hostname); }
    if (dhcp_ipv4.client) { config->setClient(*dhcp_ipv4.client); }

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv4_static>
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

static std::shared_ptr<InterfaceProtocolConfig_ipv4>
make_swagger_protocol_config(const ipv4_protocol_config& ipv4)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4>();

    auto visitor = utils::overloaded_visitor(
        [&](const ipv4_auto_protocol_config&) { config->setMethod("auto"); },
        [&](const ipv4_dhcp_protocol_config& dhcp_ipv4) {
            config->setDhcp(make_swagger_protocol_config(dhcp_ipv4));
            config->setMethod("dhcp");
        },
        [&](const ipv4_static_protocol_config& static_ipv4) {
            config->setStatic(make_swagger_protocol_config(static_ipv4));
            config->setMethod("static");
        });

    std::visit(visitor, ipv4);

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv6_dhcp6>
make_swagger_protocol_config(const ipv6_dhcp6_protocol_config& dhcp6_ipv6)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6_dhcp6>();

    config->setStateless(dhcp6_ipv6.stateless);

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv6_static>
make_swagger_protocol_config(const ipv6_static_protocol_config& static_ipv6)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6_static>();

    if (static_ipv6.gateway) {
        config->setGateway(to_string(*static_ipv6.gateway));
    }
    config->setAddress(to_string(static_ipv6.address));
    config->setPrefixLength(static_ipv6.prefix_length);

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv6>
make_swagger_protocol_config(const ipv6_protocol_config& ipv6)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6>();

    auto visitor = utils::overloaded_visitor(
        [&](const ipv6_auto_protocol_config& auto_ipv6) {
            config->setMethod("auto");
            if (auto_ipv6.link_local_address) {
                config->setLinkLocalAddress(
                    to_string(*auto_ipv6.link_local_address));
            }
        },
        [&](const ipv6_dhcp6_protocol_config& dhcp6_ipv6) {
            config->setDhcp6(make_swagger_protocol_config(dhcp6_ipv6));
            config->setMethod("dhcp6");
            if (dhcp6_ipv6.link_local_address) {
                config->setLinkLocalAddress(
                    to_string(*dhcp6_ipv6.link_local_address));
            }
        },
        [&](const ipv6_static_protocol_config& static_ipv6) {
            config->setStatic(make_swagger_protocol_config(static_ipv6));
            config->setMethod("static");
            if (static_ipv6.link_local_address) {
                config->setLinkLocalAddress(
                    to_string(*static_ipv6.link_local_address));
            }
        });

    std::visit(visitor, ipv6);

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig>
make_swagger_protocol_config(const protocol_config& protocol)
{
    auto config = std::make_shared<InterfaceProtocolConfig>();

    auto visitor = utils::overloaded_visitor(
        [&](const eth_protocol_config& eth) {
            config->setEth(make_swagger_protocol_config(eth));
        },
        [&](const ipv4_protocol_config& ipv4) {
            config->setIpv4(make_swagger_protocol_config(ipv4));
        },
        [&](const ipv6_protocol_config& ipv6) {
            config->setIpv6(make_swagger_protocol_config(ipv6));
        },
        [](const std::monostate&) {
            /* Nothing to do */
        });

    std::visit(visitor, protocol);

    return (config);
}

static std::shared_ptr<Interface_config>
make_swagger_interface_config(const config_data& in_config)
{
    auto config = std::make_shared<Interface_config>();
    for (auto& protocol : in_config.protocols) {
        config->getProtocols().emplace_back(
            make_swagger_protocol_config(protocol));
    }
    return (config);
}

static std::shared_ptr<InterfaceStats>
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

std::shared_ptr<Interface>
make_swagger_interface(const generic_interface& in_intf)
{
    auto out_intf = std::make_shared<Interface>();

    out_intf->setId(in_intf.id());
    out_intf->setPortId(in_intf.port_id());
    out_intf->setConfig(make_swagger_interface_config(in_intf.config()));
    out_intf->setStats(make_swagger_interface_stats(in_intf));

    return (out_intf);
}

} // namespace interface
} // namespace packetio
} // namespace openperf
