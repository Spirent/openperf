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

namespace openperf::packetio::interface {

using namespace swagger::v1::model;
using namespace libpacket::type;

static void validate(const eth_protocol_config& eth,
                     std::vector<std::string>& errors)
{
    if (is_broadcast(eth.address)) {
        errors.emplace_back("Broadcast MAC address (" + to_string(eth.address)
                            + ") is not allowed.");
    } else if (is_multicast(eth.address)) {
        errors.emplace_back("Multicast MAC address (" + to_string(eth.address)
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
    if (is_loopback(ipv4.address)) {
        errors.emplace_back("Cannot use loopback address ("
                            + to_string(ipv4.address) + ") for interface.");
    } else if (is_multicast(ipv4.address)) {
        errors.emplace_back("Cannot use multicast address ("
                            + to_string(ipv4.address) + ") for interface.");
    }

    /* check optional gateway */
    if (ipv4.gateway) {
        if (is_loopback(*ipv4.gateway)) {
            errors.emplace_back("Cannot use loopback address ("
                                + to_string(*ipv4.gateway) + ") for gateway.");
        } else if (is_multicast(*ipv4.gateway)) {
            errors.emplace_back("Cannot use multicast address ("
                                + to_string(*ipv4.gateway) + ") for gateway.");
        } else if (ipv4_network(*ipv4.gateway, ipv4.prefix_length)
                   != ipv4_network(ipv4.address, ipv4.prefix_length)) {
            errors.emplace_back(
                "Gateway address (" + to_string(*ipv4.gateway)
                + ") is not in the same broadcast domain as interface ("
                + to_string(ipv4_network(ipv4.address, ipv4.prefix_length))
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
        if (!is_linklocal(*config.link_local_address)) {
            errors.emplace_back("Cannot use non link local address ("
                                + to_string(*config.link_local_address)
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

    if (config.prefix_length > ipv6_network::max_prefix_length) {
        errors.emplace_back("Prefix length ("
                            + std::to_string(config.prefix_length)
                            + ") is too big.");
    }
    if (is_loopback(config.address)) {
        errors.emplace_back("Cannot use loopback address ("
                            + to_string(config.address) + ") for interface.");
    } else if (is_multicast(config.address)) {
        errors.emplace_back("Cannot use multicast address ("
                            + to_string(config.address) + ") for interface.");
    }

    /* check optional gateway */
    if (config.gateway) {
        if (is_loopback(*config.gateway)) {
            errors.emplace_back("Cannot use loopback address ("
                                + to_string(*config.gateway)
                                + ") for gateway.");
        } else if (is_multicast(*config.gateway)) {
            errors.emplace_back("Cannot use multicast address ("
                                + to_string(*config.gateway)
                                + ") for gateway.");
        } else if (ipv6_network(*config.gateway, config.prefix_length)
                   != ipv6_network(config.address, config.prefix_length)) {
            errors.emplace_back(
                "Gateway address (" + to_string(*config.gateway)
                + ") is not in the same broadcast domain as interface ("
                + to_string(ipv6_network(config.address, config.prefix_length))
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

static bool is_valid_config(config_data& config,
                            std::vector<std::string>& errors)
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
    return (
        eth_protocol_config{.address = mac_address(config->getMacAddress())});
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
            std::make_optional(ipv4_address(config->getGateway()));
    }

    to_return.address = ipv4_address(config->getAddress());
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
            std::make_optional(ipv6_address(config->getGateway()));
    }

    to_return.address = ipv6_address(config->getAddress());
    /* XXX: Can't use small ints in swagger, so cut this value down. */
    to_return.prefix_length =
        config->getPrefixLength() & std::numeric_limits<uint8_t>::max();

    return (to_return);
}

static ipv6_protocol_config
from_swagger(const std::shared_ptr<InterfaceProtocolConfig_ipv6>& config)
{
    ipv6_protocol_config to_return;
    std::optional<ipv6_address> link_local_address;

    if (config->linkLocalAddressIsSet()) {
        link_local_address =
            std::make_optional(ipv6_address(config->getLinkLocalAddress()));
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
    if (!is_valid_config(to_return, errors)) {
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

template <typename Key, typename Value, typename... Pairs>
constexpr auto associative_array(Pairs&&... pairs)
    -> std::array<std::pair<Key, Value>, sizeof...(pairs)>
{
    return {{std::forward<Pairs>(pairs)...}};
}

static std::string to_string(enum dhcp_client_state state)
{
    static const auto state_pairs =
        associative_array<dhcp_client_state, std::string>(
            std::pair(dhcp_client_state::none, "none"),
            std::pair(dhcp_client_state::rebooting, "rebooting"),
            std::pair(dhcp_client_state::init_reboot, "init_reboot"),
            std::pair(dhcp_client_state::init, "init"),
            std::pair(dhcp_client_state::selecting, "selecting"),
            std::pair(dhcp_client_state::requesting, "requesting"),
            std::pair(dhcp_client_state::checking, "checking"),
            std::pair(dhcp_client_state::bound, "bound"),
            std::pair(dhcp_client_state::renewing, "renewing"),
            std::pair(dhcp_client_state::rebinding, "rebinding"));

    auto cursor = std::begin(state_pairs), end = std::end(state_pairs);
    while (cursor != end) {
        if (cursor->first == state) { return (cursor->second); }
        cursor++;
    }

    return ("unknown");
}

static std::string to_string(enum ipv6_address_state state)
{
    static const auto state_pairs =
        associative_array<ipv6_address_state, std::string>(
            std::pair(ipv6_address_state::none, "none"),
            std::pair(ipv6_address_state::invalid, "invalid"),
            std::pair(ipv6_address_state::tentative, "tentative"),
            std::pair(ipv6_address_state::preferred, "preferred"),
            std::pair(ipv6_address_state::deprecated, "deprecated"),
            std::pair(ipv6_address_state::duplicated, "duplicated"));

    auto cursor = std::begin(state_pairs), end = std::end(state_pairs);
    while (cursor != end) {
        if (cursor->first == state) { return (cursor->second); }
        cursor++;
    }

    return ("unknown");
}

static std::shared_ptr<InterfaceProtocolConfig_ipv4_dhcp>
make_swagger_protocol_config(const ipv4_dhcp_protocol_config& dhcp_ipv4,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4_dhcp>();
    if (dhcp_ipv4.hostname) { config->setHostname(*dhcp_ipv4.hostname); }
    if (dhcp_ipv4.client) { config->setClient(*dhcp_ipv4.client); }

    auto status = std::make_shared<InterfaceProtocolConfig_ipv4_dhcp_status>();
    status->setState(to_string(intf.dhcp_state()));
    if (auto addr = intf.ipv4_address()) { status->setAddress(addr.value()); }
    if (auto gw = intf.ipv4_gateway()) { status->setGateway(gw.value()); }
    if (auto prefix = intf.ipv4_prefix_length()) {
        status->setPrefixLength(prefix.value());
    }
    config->setStatus(status);

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
make_swagger_protocol_config(const ipv4_protocol_config& ipv4,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv4>();

    auto visitor = utils::overloaded_visitor(
        [&](const ipv4_auto_protocol_config&) { config->setMethod("auto"); },
        [&](const ipv4_dhcp_protocol_config& dhcp_ipv4) {
            config->setDhcp(make_swagger_protocol_config(dhcp_ipv4, intf));
            config->setMethod("dhcp");
        },
        [&](const ipv4_static_protocol_config& static_ipv4) {
            config->setStatic(make_swagger_protocol_config(static_ipv4));
            config->setMethod("static");
        });

    std::visit(visitor, ipv4);

    return (config);
}

static std::shared_ptr<Ipv6DynamicAddressStatus>
make_ipv6_dynamic_address_status(const generic_interface& intf)
{
    auto status = std::make_shared<Ipv6DynamicAddressStatus>();

    status->setState(to_string(intf.ipv6_state()));
    if (auto addr = intf.ipv6_address()) { status->setAddress(addr.value()); }
    if (auto scope = intf.ipv6_scope()) { status->setScope(scope.value()); }

    return (status);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv6_auto>
make_swagger_protocol_config(const ipv6_auto_protocol_config&,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6_auto>();
    config->setStatus(make_ipv6_dynamic_address_status(intf));

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig_ipv6_dhcp6>
make_swagger_protocol_config(const ipv6_dhcp6_protocol_config& dhcp6_ipv6,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6_dhcp6>();

    config->setStateless(dhcp6_ipv6.stateless);
    config->setStatus(make_ipv6_dynamic_address_status(intf));

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
make_swagger_protocol_config(const ipv6_protocol_config& ipv6,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig_ipv6>();
    if (auto linklocal = intf.ipv6_linklocal_address()) {
        config->setLinkLocalAddress(linklocal.value());
    }

    auto visitor = utils::overloaded_visitor(
        [&](const ipv6_auto_protocol_config& auto_ipv6) {
            config->setAuto(make_swagger_protocol_config(auto_ipv6, intf));
            config->setMethod("auto");
        },
        [&](const ipv6_dhcp6_protocol_config& dhcp6_ipv6) {
            config->setDhcp6(make_swagger_protocol_config(dhcp6_ipv6, intf));
            config->setMethod("dhcp6");
        },
        [&](const ipv6_static_protocol_config& static_ipv6) {
            config->setStatic(make_swagger_protocol_config(static_ipv6));
            config->setMethod("static");
        });

    std::visit(visitor, ipv6);

    return (config);
}

static std::shared_ptr<InterfaceProtocolConfig>
make_swagger_protocol_config(const protocol_config& protocol,
                             const generic_interface& intf)
{
    auto config = std::make_shared<InterfaceProtocolConfig>();

    auto visitor = utils::overloaded_visitor(
        [&](const eth_protocol_config& eth) {
            config->setEth(make_swagger_protocol_config(eth));
        },
        [&](const ipv4_protocol_config& ipv4) {
            config->setIpv4(make_swagger_protocol_config(ipv4, intf));
        },
        [&](const ipv6_protocol_config& ipv6) {
            config->setIpv6(make_swagger_protocol_config(ipv6, intf));
        },
        [](const std::monostate&) {
            /* Nothing to do */
        });

    std::visit(visitor, protocol);

    return (config);
}

static std::shared_ptr<Interface_config>
make_swagger_interface_config(const generic_interface& intf)
{
    auto config = std::make_shared<Interface_config>();
    for (auto& protocol : intf.config().protocols) {
        config->getProtocols().emplace_back(
            make_swagger_protocol_config(protocol, intf));
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

std::unique_ptr<Interface>
make_swagger_interface(const generic_interface& in_intf)
{
    auto out_intf = std::make_unique<Interface>();

    out_intf->setId(in_intf.id());
    out_intf->setPortId(in_intf.port_id());
    out_intf->setConfig(make_swagger_interface_config(in_intf));
    out_intf->setStats(make_swagger_interface_stats(in_intf));

    return (out_intf);
}

bool is_valid(const Interface& interface, std::vector<std::string>& errors)
{
    auto config = config_data{};
    if (auto if_config = interface.getConfig()) {
        for (auto& protocol : if_config->getProtocols()) {
            try {
                config.protocols.emplace_back(from_swagger(protocol));
            } catch (const std::runtime_error& e) {
                errors.emplace_back(e.what());
                return (false);
            }
        }
    }

    /* Set id field to what came in from the REST API */
    config.id = interface.getId();

    config.port_id = interface.getPortId();

    return (is_valid_config(config, errors));
}

} // namespace openperf::packetio::interface
