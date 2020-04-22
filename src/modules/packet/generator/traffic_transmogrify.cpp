#include <typeindex>

#include "packet/generator/api.hpp"
#include "packet/generator/traffic/header/config.hpp"
#include "packet/generator/traffic/length.hpp"
#include "packet/protocol/transmogrify/protocols.hpp"

#include "swagger/v1/model/TrafficDefinition.h"
#include "swagger/v1/model/TrafficLength.h"
#include "swagger/v1/model/TrafficProtocol.h"

namespace openperf::packet::generator::api {

using namespace openperf::packet::protocol::transmogrify;
using namespace openperf::packet::generator::traffic;

/**
 * Utils
 */

static header::modifier_mux to_modifier_mux(enum api::mux_type in)
{
    return (in == api::mux_type::cartesian ? header::modifier_mux::cartesian
                                           : header::modifier_mux::zip);
}

static header::modifier_mux
to_modifier_mux(const swagger::v1::model::TrafficProtocol& protocol)
{
    if (!protocol.modifiersIsSet() || !protocol.getModifiers()->tieIsSet()) {
        return (header::modifier_mux::zip);
    }

    return (
        to_modifier_mux(api::to_mux_type(protocol.getModifiers()->getTie())));
}

static header::modifier_mux
to_modifier_mux(const swagger::v1::model::TrafficPacketTemplate& packet)
{
    if (!packet.modifierTieIsSet()) { return (header::modifier_mux::zip); }

    return (to_modifier_mux(api::to_mux_type(packet.getModifierTie())));
}

/**
 * Traffic sequence transformations
 */

namespace detail {

template <typename Field, typename ModifierSequence>
modifier::config to_modifier_seq_config(const ModifierSequence& sequence)
{
    auto mod_conf = modifier::sequence_config<Field>{
        .first = Field(sequence.getStart()),
        .count = static_cast<unsigned>(sequence.getCount())};

    if (sequence.stopIsSet()) { mod_conf.last = Field(sequence.getStop()); }

    const auto& skip_list = const_cast<ModifierSequence&>(sequence).getSkip();
    std::transform(std::begin(skip_list),
                   std::end(skip_list),
                   std::back_inserter(mod_conf.skip),
                   [](const auto& value) { return (Field(value)); });

    return (mod_conf);
}

template <typename Field, typename ModifierList>
modifier::config to_modifier_list_config(const ModifierList& list)
{
    auto mod_conf = modifier::list_config<Field>{};

    std::transform(std::begin(list),
                   std::end(list),
                   std::back_inserter(mod_conf.items),
                   [](const auto& value) { return (Field(value)); });

    return (mod_conf);
}

template <typename Field, typename FieldModifier>
modifier::config to_modifier_config(const FieldModifier& modifier)
{
    return (modifier.listIsSet()
                ? to_modifier_list_config<Field>(
                    const_cast<FieldModifier&>(modifier).getList())
                : to_modifier_seq_config<Field>(*(modifier.getSequence())));
}

} // namespace detail

template <typename Protocol>
modifier::config
to_modifier_config(const swagger::v1::model::TrafficProtocolModifier& modifier)
{
    auto field_name = Protocol::get_field_name(modifier.getName());
    auto& field_type = Protocol::get_field_type(field_name);

    if (field_type == typeid(libpacket::type::ipv4_address)) {
        assert(modifier.ipv4IsSet());
        return (detail::to_modifier_config<libpacket::type::ipv4_address>(
            *modifier.getIpv4()));
    } else if (field_type == typeid(libpacket::type::ipv6_address)) {
        assert(modifier.ipv6IsSet());
        return (detail::to_modifier_config<libpacket::type::ipv6_address>(
            *modifier.getIpv6()));
    } else if (field_type == typeid(libpacket::type::mac_address)) {
        assert(modifier.macIsSet());
        return (detail::to_modifier_config<libpacket::type::mac_address>(
            *modifier.getMac()));
    } else {
        assert(modifier.fieldIsSet());
        return (detail::to_modifier_config<uint32_t>(*modifier.getField()));
    }
}

template <typename Protocol>
auto to_modifier_container(const swagger::v1::model::TrafficProtocol& protocol)
{
    using config = header::config<Protocol>;
    auto modifiers = typename config::modifier_container{};
    if (protocol.modifiersIsSet()) {
        const auto& api_mod_config = protocol.getModifiers();
        const auto& api_modifiers = api_mod_config->getItems();
        std::transform(std::begin(api_modifiers),
                       std::end(api_modifiers),
                       std::back_inserter(modifiers),
                       [](const auto& api_mod) {
                           return std::make_pair(
                               Protocol::get_field_name(api_mod->getName()),
                               to_modifier_config<Protocol>(*api_mod));
                       });
    }
    return (modifiers);
}

static header::config_instance
to_header_config_ethernet(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::ethernet_config{
        to_protocol(protocol.getEthernet()),
        to_modifier_container<libpacket::protocol::ethernet>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_mpls(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::mpls_config{
        to_protocol(protocol.getMpls()),
        to_modifier_container<libpacket::protocol::mpls>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_vlan(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::vlan_config{
        to_protocol(protocol.getVlan()),
        to_modifier_container<libpacket::protocol::vlan>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_ipv4(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::ipv4_config{
        to_protocol(protocol.getIpv4()),
        to_modifier_container<libpacket::protocol::ipv4>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_ipv6(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::ipv6_config{
        to_protocol(protocol.getIpv6()),
        to_modifier_container<libpacket::protocol::ipv6>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_tcp(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::tcp_config{
        to_protocol(protocol.getTcp()),
        to_modifier_container<libpacket::protocol::tcp>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config_udp(const swagger::v1::model::TrafficProtocol& protocol)
{
    return (header::udp_config{
        to_protocol(protocol.getUdp()),
        to_modifier_container<libpacket::protocol::udp>(protocol),
        to_modifier_mux(protocol)});
}

static header::config_instance
to_header_config(const swagger::v1::model::TrafficProtocol& protocol)
{
    if (protocol.ethernetIsSet()) {
        return (to_header_config_ethernet(protocol));
    } else if (protocol.mplsIsSet()) {
        return (to_header_config_mpls(protocol));
    } else if (protocol.vlanIsSet()) {
        return (to_header_config_vlan(protocol));
    } else if (protocol.ipv4IsSet()) {
        return (to_header_config_ipv4(protocol));
    } else if (protocol.ipv6IsSet()) {
        return (to_header_config_ipv6(protocol));
    } else if (protocol.tcpIsSet()) {
        return (to_header_config_tcp(protocol));
    } else {
        assert(protocol.udpIsSet());
        return (to_header_config_udp(protocol));
    }
}

/**
 * Length transformation
 */

static traffic::length_config
to_length_config(const swagger::v1::model::TrafficLength& length)
{
    if (length.fixedIsSet()) {
        return (traffic::length_fixed_config{
            static_cast<uint16_t>(length.getFixed())});
    } else if (length.listIsSet()) {
        auto config = traffic::length_list_config{};
        const auto& lengths =
            const_cast<swagger::v1::model::TrafficLength&>(length).getList();
        std::transform(
            std::begin(lengths),
            std::end(lengths),
            std::back_inserter(config.items),
            [](const auto& l) { return (static_cast<uint16_t>(l)); });
        return (config);
    } else {
        assert(length.sequenceIsSet());
        const auto& seq = length.getSequence();
        auto config = traffic::length_sequence_config{
            .first = static_cast<uint16_t>(seq->getStart()),
            .count = static_cast<unsigned>(seq->getCount())};

        if (seq->stopIsSet()) {
            config.last = static_cast<uint16_t>(seq->getStop());
        }

        const auto& skips = seq->getSkip();
        std::transform(
            std::begin(skips),
            std::end(skips),
            std::back_inserter(config.skip),
            [](const auto& l) { return (static_cast<uint16_t>(l)); });

        return (config);
    }
}

/**
 * Traffic Definition transformation
 */

traffic::packet_template
to_packet_template(const swagger::v1::model::TrafficDefinition& definition)
{
    auto headers = traffic::header::config_container{};
    const auto& pkt = definition.getPacket();
    const auto& protocols = pkt->getProtocols();
    std::transform(
        std::begin(protocols),
        std::end(protocols),
        std::back_inserter(headers),
        [](const auto& protocol) { return (to_header_config(*protocol)); });

    return (traffic::packet_template(
        traffic::header::update_context_fields(std::move(headers)),
        to_modifier_mux(*pkt),
        traffic::get_lengths(to_length_config(*definition.getLength()))));
}

} // namespace openperf::packet::generator::api
