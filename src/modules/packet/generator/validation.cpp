#include <iostream>
#include "packet/generator/api.hpp"
#include "packet/protocol/transmogrify/protocols.hpp"

#include "swagger/v1/model/PacketGenerator.h"

namespace openperf::packet::generator::api {

using generator_config_ptr =
    std::shared_ptr<swagger::v1::model::PacketGeneratorConfig>;
using definition_ptr = std::shared_ptr<swagger::v1::model::TrafficDefinition>;
using duration_ptr = std::shared_ptr<swagger::v1::model::TrafficDuration>;
using length_ptr = std::shared_ptr<swagger::v1::model::TrafficLength>;
using load_ptr = std::shared_ptr<swagger::v1::model::TrafficLoad>;
using modifier_ptr =
    std::shared_ptr<swagger::v1::model::TrafficProtocolModifier>;
using field_modifier_ptr =
    std::shared_ptr<swagger::v1::model::TrafficProtocolFieldModifier>;
using packet_template_ptr =
    std::shared_ptr<swagger::v1::model::TrafficPacketTemplate>;
using protocol_ptr = std::shared_ptr<swagger::v1::model::TrafficProtocol>;
using signature_ptr = std::shared_ptr<swagger::v1::model::SpirentSignature>;

const std::string id_string(const size_t def_idx)
{
    return (" on traffic definition " + std::to_string(def_idx));
}

const std::string id_string(const size_t def_idx, const size_t pkt_idx)
{
    return (id_string(def_idx) + ", packet index " + std::to_string(pkt_idx));
}

const std::string
id_string(const size_t def_idx, const size_t pkt_idx, const size_t proto_idx)
{
    return (id_string(def_idx, pkt_idx) + ", protocol index "
            + std::to_string(proto_idx));
}

const std::string id_string(const size_t def_idx,
                            const size_t pkt_idx,
                            const size_t proto_idx,
                            const size_t mod_idx)
{
    return (id_string(def_idx, pkt_idx, proto_idx) + ", modifier index "
            + std::to_string(mod_idx));
}

const std::string id_string(const size_t def_idx,
                            const size_t pkt_idx,
                            const size_t proto_idx,
                            const size_t mod_idx,
                            std::string_view field_name)
{
    return (id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ", field name "
            + std::string(field_name));
}

template <typename T>
struct is_string : public std::is_same<std::string, std::decay_t<T>>
{};

template <typename T> inline constexpr bool is_string_v = is_string<T>::value;

template <typename Field, typename Value>
std::enable_if_t<!is_string_v<Value>, bool> is_valid(const Value& value)
{
    return (value > Value{0});
}

template <typename Field, typename Value>
std::enable_if_t<is_string_v<Value>, bool> is_valid(const Value& value)
{
    try {
        [[maybe_unused]] auto x = Field(value);
        return (true);
    } catch (const std::runtime_error&) {
        return (false);
    }
}

template <typename T>
std::enable_if_t<!is_string_v<T>, const std::string> to_string(const T& value)
{
    return (std::to_string(value));
}

template <typename T>
std::enable_if_t<is_string_v<T>, const std::string> to_string(const T& value)
{
    return (value);
}

template <typename Field, typename ModifierSequence>
static void validate_field_modifier_sequence(
    const std::shared_ptr<ModifierSequence>& sequence,
    std::string_view field_name,
    const size_t def_idx,
    const size_t pkt_idx,
    const size_t proto_idx,
    const size_t mod_idx,
    std::vector<std::string>& errors)
{
    if (sequence->getCount() < 1) {
        errors.emplace_back(
            "Sequence count must be positive "
            + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
            + ".");
    }

    if (!is_valid<Field>(sequence->getStart())) {
        errors.emplace_back(
            "Sequence start value, " + to_string(sequence->getStart())
            + ", is invalid"
            + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
            + ".");
    }

    if (sequence->stopIsSet() && !is_valid<Field>(sequence->getStop())) {
        errors.emplace_back(
            "Sequence stop value, " + to_string(sequence->getStop())
            + ", is invalid"
            + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
            + ".");
    }

    for (const auto& item : sequence->getSkip()) {
        if (!is_valid<Field>(item)) {
            errors.emplace_back(
                "Sequence skip item, " + to_string(item) + ", is invalid"
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
                + ".");
        }
    }
}

template <typename Field, typename ModifierContainer>
static void validate_field_modifier_list(const ModifierContainer& list,
                                         std::string_view field_name,
                                         const size_t def_idx,
                                         const size_t pkt_idx,
                                         const size_t proto_idx,
                                         const size_t mod_idx,
                                         std::vector<std::string>& errors)
{
    std::for_each(std::begin(list), std::end(list), [&](const auto& item) {
        if (!is_valid<Field>(item)) {
            errors.emplace_back(
                "Modifier list item, " + to_string(item) + ", is invalid"
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
                + ".");
        }
    });
}

template <typename Field, typename ModifierType>
static void
validate_field_modifier(const std::shared_ptr<ModifierType>& field_mod,
                        std::string_view field_name,
                        const size_t def_idx,
                        const size_t pkt_idx,
                        const size_t proto_idx,
                        const size_t mod_idx,
                        std::vector<std::string>& errors)
{
    assert(field_mod);

    if (field_mod->listIsSet()) {
        validate_field_modifier_list<Field>(field_mod->getList(),
                                            field_name,
                                            def_idx,
                                            pkt_idx,
                                            proto_idx,
                                            mod_idx,
                                            errors);
    } else if (field_mod->sequenceIsSet()) {
        validate_field_modifier_sequence<Field>(field_mod->getSequence(),
                                                field_name,
                                                def_idx,
                                                pkt_idx,
                                                proto_idx,
                                                mod_idx,
                                                errors);
    } else {
        errors.emplace_back(
            "Modifier has no configuration"
            + id_string(def_idx, pkt_idx, proto_idx, mod_idx, field_name)
            + ".");
    }
}

template <typename Protocol>
static void validate_modifier(const modifier_ptr& modifier,
                              const size_t def_idx,
                              const size_t pkt_idx,
                              const size_t proto_idx,
                              const size_t mod_idx,
                              std::vector<std::string>& errors)
{
    auto field_name = Protocol::get_field_name(modifier->getName());
    if (field_name == Protocol::field_name::none) {
        errors.emplace_back(
            "Modifier field name, " + modifier->getName() + ", is invalid"
            + id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ".");
        return;
    }

    auto& field_type = Protocol::get_field_type(field_name);

    if (field_type == typeid(libpacket::type::ipv4_address)) {
        if (!modifier->ipv4IsSet()) {
            errors.emplace_back(
                "Missing IPv4 address modifier for field " + modifier->getName()
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ".");
        } else {
            validate_field_modifier<libpacket::type::ipv4_address>(
                modifier->getIpv4(),
                modifier->getName(),
                def_idx,
                pkt_idx,
                proto_idx,
                mod_idx,
                errors);
        }
    } else if (field_type == typeid(libpacket::type::ipv6_address)) {
        if (!modifier->ipv6IsSet()) {
            errors.emplace_back(
                "Missing IPv6 address modifier for field " + modifier->getName()
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ".");
        } else {
            validate_field_modifier<libpacket::type::ipv6_address>(
                modifier->getIpv6(),
                modifier->getName(),
                def_idx,
                pkt_idx,
                proto_idx,
                mod_idx,
                errors);
        }
    } else if (field_type == typeid(libpacket::type::mac_address)) {
        if (!modifier->macIsSet()) {
            errors.emplace_back(
                "Missing MAC address modifier for field " + modifier->getName()
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ".");
        } else {
            validate_field_modifier<libpacket::type::mac_address>(
                modifier->getMac(),
                modifier->getName(),
                def_idx,
                pkt_idx,
                proto_idx,
                mod_idx,
                errors);
        }
    } else {
        if (!modifier->fieldIsSet()) {
            errors.emplace_back(
                "Missing modifier for field " + modifier->getName()
                + id_string(def_idx, pkt_idx, proto_idx, mod_idx) + ".");
        } else {
            validate_field_modifier<uint32_t>(modifier->getField(),
                                              modifier->getName(),
                                              def_idx,
                                              pkt_idx,
                                              proto_idx,
                                              mod_idx,
                                              errors);
        }
    }
}

/*
 * XXX: This protocol validation is rather unpleasant.
 * The basic idea is that we have automatically generated code that knows
 * about each protocol type and how to convert between the 'on-the-wire' and
 * swagger versions of them. Additionally, the swagger protocol objects
 * have method to check for the presence of specific protocol sub-types as
 * well as methods to access them.
 *
 * So, in order to validate each specific protocol type, we use a templated
 * function that take a function parameter to retrieve a generic pointer
 * to the specific protocol configuration. We then use our template knowledge
 * to circuitously determine the actual swagger type and cast the base pointer
 * back to the real protocol type. Finally, we try to convert the swagger
 * configuration to the 'on-the-wire' format and catch any errors.
 *
 * Note: the get_protocol_fn returns the swagger base type so that we
 * have a common type we can store in our protocol handlers array.
 *
 * It's not very pretty, but I'm willing to accept improvements!
 */

using has_protocol_fn = bool (swagger::v1::model::TrafficProtocol::*)() const;
using get_protocol_fn =
    std::function<std::shared_ptr<swagger::v1::model::ModelBase>(
        const protocol_ptr&)>;
using validate_protocol_fn = std::function<void(const get_protocol_fn&,
                                                const protocol_ptr&,
                                                size_t,
                                                size_t,
                                                size_t,
                                                std::vector<std::string>&)>;

template <typename Protocol>
static void validate_protocol(const get_protocol_fn& get_fn,
                              const protocol_ptr& protocol,
                              const size_t def_idx,
                              const size_t pkt_idx,
                              const size_t proto_idx,
                              std::vector<std::string>& errors)
{
    using swagger_type = typename decltype(protocol::transmogrify::to_swagger(
        std::declval<Protocol&>()))::element_type;
    const auto& swagger_protocol =
        std::static_pointer_cast<swagger_type>(get_fn(protocol));

    /*
     * This function will throw if there are any conversion errors from the
     * swagger object to the wire object.  So, if we catch anything, then the
     * swagger object has an error.
     */
    try {
        protocol::transmogrify::to_protocol(swagger_protocol);
    } catch (const std::runtime_error& e) {
        errors.emplace_back("Invalid configuration for "
                            + std::string(Protocol::protocol_name) + " protocol"
                            + id_string(def_idx, pkt_idx, proto_idx) + ": "
                            + e.what() + ".");
    }

    if (protocol->modifiersIsSet()) {
        const auto& modifiers_conf = protocol->getModifiers();
        const auto& modifiers = modifiers_conf->getItems();

        if (modifiers_conf->tieIsSet()) {
            if (to_mux_type(modifiers_conf->getTie()) == mux_type::none) {
                errors.emplace_back("Modifier tie, " + modifiers_conf->getTie()
                                    + ", is unrecognized"
                                    + id_string(def_idx, pkt_idx, proto_idx)
                                    + ".");
            }
        } else if (modifiers.size() > 1) {
            errors.emplace_back(
                "Modifier tie must be set when multiple modifiers are present"
                + id_string(def_idx, pkt_idx, proto_idx) + ".");
        }

        auto mod_idx = 1U;
        for (const auto& mod : modifiers) {
            validate_modifier<Protocol>(
                mod, def_idx, pkt_idx, proto_idx, mod_idx++, errors);
        }
    }
}

template <typename HasFn,
          typename ValidateFn,
          typename GetFn,
          typename... Tuples>
constexpr auto protocol_handler_array(Tuples&&... tuples)
    -> std::array<std::tuple<HasFn, ValidateFn, GetFn>, sizeof...(tuples)>
{
    return {{std::forward<Tuples>(tuples)...}};
}

/*
 * Store all the function pointers we need to perform the protocol validation in
 * an array. We use a macro to simplify the individual element generation. As
 * far as the two name parameters, well...  it's just more proof that camelCase
 * is evil.
 */
#define MAKE_PROTOCOL_TUPLE(name1, name2)                                      \
    std::make_tuple(                                                           \
        &swagger::v1::model::TrafficProtocol::name1##IsSet,                    \
        &validate_protocol<libpacket::protocol::name1>,                        \
        [](const protocol_ptr& p) {                                            \
            return (std::static_pointer_cast<swagger::v1::model::ModelBase>(   \
                p->get##name2()));                                             \
        })

static const auto protocol_handlers =
    protocol_handler_array<has_protocol_fn,
                           validate_protocol_fn,
                           get_protocol_fn>(
        MAKE_PROTOCOL_TUPLE(ethernet, Ethernet),
        MAKE_PROTOCOL_TUPLE(mpls, Mpls),
        MAKE_PROTOCOL_TUPLE(vlan, Vlan),
        MAKE_PROTOCOL_TUPLE(ipv4, Ipv4),
        MAKE_PROTOCOL_TUPLE(ipv6, Ipv6),
        MAKE_PROTOCOL_TUPLE(tcp, Tcp),
        MAKE_PROTOCOL_TUPLE(udp, Udp));

#undef MAKE_PROTOCOL_TUPLE

static void validate(const packet_template_ptr& packet,
                     const size_t def_idx,
                     const size_t pkt_idx,
                     std::vector<std::string>& errors)
{
    if (packet->getProtocols().empty()) {
        errors.emplace_back("At least one protocol is required"
                            + id_string(def_idx, pkt_idx) + ".");
    } else {
        auto proto_idx = 1U;
        for (const auto& proto : packet->getProtocols()) {
            /**
             * Weird syntax alert.  Use ".*" to call the member function
             * stored in the pair.
             */
            auto cursor = std::find_if(std::begin(protocol_handlers),
                                       std::end(protocol_handlers),
                                       [&](const auto& item) {
                                           return (*proto.*std::get<0>(item))();
                                       });
            if (cursor != std::end(protocol_handlers)) {
                std::get<1> (*cursor)(std::get<2>(*cursor),
                                      proto,
                                      def_idx,
                                      pkt_idx,
                                      proto_idx,
                                      errors);
            } else {
                errors.emplace_back("No protocol configuration found"
                                    + id_string(def_idx, pkt_idx, proto_idx)
                                    + ".");
            }
            proto_idx++;
        }
    }

    /* Make sure modifier tie is valid */
    if (packet->modifierTieIsSet()
        && to_mux_type(packet->getModifierTie()) == mux_type::none) {
        errors.emplace_back("Modifier tie," + packet->getModifierTie()
                            + ", is invalid" + id_string(def_idx, pkt_idx)
                            + ".");
    }
}

template <typename FieldModifier>
static std::optional<size_t> get_modifier_length(const FieldModifier& mod)
{
    if (mod->listIsSet()) {
        return (mod->getList().size());
    } else if (mod->sequenceIsSet()) {
        return (mod->getSequence()->getCount());
    } else {
        return {};
    }
}

static std::optional<size_t> get_modifier_length(const modifier_ptr& modifier)
{
    if (modifier->fieldIsSet()) {
        return (get_modifier_length(modifier->getField()));
    } else if (modifier->ipv4IsSet()) {
        return (get_modifier_length(modifier->getIpv4()));
    } else if (modifier->ipv6IsSet()) {
        return (get_modifier_length(modifier->getIpv6()));
    } else if (modifier->macIsSet()) {
        return (get_modifier_length(modifier->getMac()));
    } else {
        return {};
    }
}

/*
 * If the total is less than any value in the container, then
 * we must have had an overflow.
 */
template <typename Container, typename T>
bool has_overflow(const Container& values, T total)
{
    return (std::any_of(std::begin(values),
                        std::end(values),
                        [&](const auto& x) { return (total < x); }));
}

static size_t count_headers(const protocol_ptr& protocol,
                            const size_t def_idx,
                            const size_t pkt_idx,
                            const size_t proto_idx,
                            std::vector<std::string>& errors)
{
    if (!protocol->modifiersIsSet()) { return (1); }

    const auto& mod_container = protocol->getModifiers();
    const auto& modifiers = mod_container->getItems();
    std::vector<std::optional<size_t>> modifier_lengths{};
    std::transform(
        std::begin(modifiers),
        std::end(modifiers),
        std::back_inserter(modifier_lengths),
        [](const auto& modifier) { return (get_modifier_length(modifier)); });

    auto mux_type = mod_container->tieIsSet()
                        ? to_mux_type(mod_container->getTie())
                        : mux_type::zip;

    auto total = (mux_type == mux_type::zip
                      ? std::accumulate(std::begin(modifier_lengths),
                                        std::end(modifier_lengths),
                                        0UL,
                                        [](size_t sum, const auto& opt) {
                                            return (sum += opt.value_or(0));
                                        })
                      : std::accumulate(std::begin(modifier_lengths),
                                        std::end(modifier_lengths),
                                        1UL,
                                        [](size_t product, const auto& opt) {
                                            return (product *= opt.value_or(1));
                                        }));

    if (has_overflow(modifier_lengths, total)) {
        errors.emplace_back("Modifier expansion results in index overflow"
                            + id_string(def_idx, pkt_idx, proto_idx) + ".");
    }

    return (total);
}

static size_t count_flows(const packet_template_ptr& packet,
                          const size_t def_idx,
                          const size_t pkt_idx,
                          std::vector<std::string>& errors)
{
    const auto& protocols = packet->getProtocols();
    std::vector<size_t> header_lengths{};
    auto proto_idx = 1U;
    std::transform(std::begin(protocols),
                   std::end(protocols),
                   std::back_inserter(header_lengths),
                   [&](const auto& protocol) {
                       return (count_headers(
                           protocol, def_idx, pkt_idx, proto_idx++, errors));
                   });

    auto mux_type = packet->modifierTieIsSet()
                        ? to_mux_type(packet->getModifierTie())
                        : mux_type::zip;

    auto total =
        (mux_type == mux_type::zip ? std::accumulate(std::begin(header_lengths),
                                                     std::end(header_lengths),
                                                     1UL,
                                                     std::lcm<size_t, size_t>)
                                   : std::accumulate(std::begin(header_lengths),
                                                     std::end(header_lengths),
                                                     1UL,
                                                     std::multiplies<>{}));

    if (has_overflow(header_lengths, total)) {
        errors.emplace_back(
            "Packet template expansion results in index overflow"
            + id_string(def_idx, pkt_idx) + ".");
    }

    return (total);
}

static void validate(const length_ptr& length,
                     const size_t def_idx,
                     std::vector<std::string>& errors)
{
    if (length->fixedIsSet()) {
        if (length->getFixed() < api::min_packet_length) {
            errors.emplace_back("Fixed length value" + id_string(def_idx)
                                + " is less that minimum frame size of "
                                + std::to_string(api::min_packet_length) + ".");
        }
        if (length->getFixed() > api::max_packet_length) {
            errors.emplace_back("Fixed length value" + id_string(def_idx)
                                + " is greater that maximum frame size of "
                                + std::to_string(api::max_packet_length) + ".");
        }
    } else if (length->listIsSet()) {
        const auto& lengths = length->getList();
        if (lengths.empty()) {
            errors.emplace_back("Length list" + id_string(def_idx)
                                + " is empty.");
        } else {
            auto&& [min, max] =
                minmax_element(std::begin(lengths), std::end(lengths));
            if (*min < api::min_packet_length) {
                errors.emplace_back(
                    "Length list" + id_string(def_idx)
                    + " contains values less that minimum frame size of "
                    + std ::to_string(api::min_packet_length) + ".");
            }
            if (*max > api::max_packet_length) {
                errors.emplace_back(
                    "Length list " + id_string(def_idx)
                    + " contains values greater that maximum frame size of "
                    + std ::to_string(api::min_packet_length) + ".");
            }
        }
    } else if (length->sequenceIsSet()) {
        auto seq = length->getSequence();
        if (!seq) {
            errors.emplace_back("Length of traffic definition "
                                + std::to_string(def_idx)
                                + " is missing sequence configuration.");
        } else {
            if (seq->getCount() < 1) {
                errors.emplace_back("Length sequence count" + id_string(def_idx)
                                    + " must be positive.");
            }
            if (seq->getStart() < api::min_packet_length) {
                errors.emplace_back(
                    "Length sequence start" + id_string(def_idx)
                    + " must be greater than minimum length value of "
                    + std::to_string(api::min_packet_length) + ".");
            }
            if (seq->stopIsSet() && seq->getStop() > api::max_packet_length) {
                errors.emplace_back(
                    "Length sequence stop" + id_string(def_idx)
                    + " must be less than maximum length value of "
                    + std::to_string(api::max_packet_length) + ".");
            }
        }
    } else {
        errors.emplace_back("Invalid length configuration found"
                            + id_string(def_idx) + ".");
    }
}

static void validate(const signature_ptr& sig, std::vector<std::string>& errors)
{
    if (sig->getStreamId() < 1) {
        errors.emplace_back("Stream ID must be positive.");
    }

    if (sig->fillIsSet()) {
        const auto& fill = sig->getFill();
        if (fill->constantIsSet()) {
            if (static_cast<unsigned>(fill->getConstant()) > 65535) {
                errors.emplace_back(
                    "Constant fill value must be less than 65535.");
            }
        } else if (fill->decrementIsSet()) {
            if (static_cast<unsigned>(fill->getDecrement()) > 255) {
                errors.emplace_back(
                    "Decrement fill value must be less than 255.");
            }
        } else if (fill->incrementIsSet()) {
            if (static_cast<unsigned>(fill->getIncrement()) > 255) {
                errors.emplace_back(
                    "Increment fill value must be less than 255.");
            }
        } else if (fill->prbsIsSet() && !fill->isPrbs()) {
            errors.emplace_back("Invalid fill configuration.");
        }
    }

    if (api::to_signature_latency_type(sig->getLatency())
        == api::signature_latency_type::none) {
        errors.emplace_back("Latency setting, " + sig->getLatency()
                            + ", is invalid.");
    }
}

static void validate(const definition_ptr& def,
                     const size_t def_idx,
                     std::vector<std::string>& errors)
{
    auto pkt_idx = 1U;
    auto nb_flows = 0ULL;
    const auto& packet = def->getPacket();
    if (!packet) {
        errors.emplace_back("Packet template is required" + id_string(def_idx)
                            + ".");
    } else {
        nb_flows += count_flows(packet, def_idx, pkt_idx, errors);
        validate(packet, def_idx, pkt_idx++, errors);
    }

    auto length = def->getLength();
    if (!length) {
        errors.emplace_back("Length object is required" + id_string(def_idx)
                            + ".");
    } else {
        validate(length, def_idx, errors);
    }

    /* Weight cannot be negative */
    if (def->weightIsSet() && def->getWeight() <= 0) {
        errors.emplace_back("Weight must be positive" + id_string(def_idx)
                            + ".");
    }

    /*
     * Flow limit depends on presence of a signature configuration. The
     * signature only has room for 64k flow indexes.
     */
    if (def->signatureIsSet()) {
        validate(def->getSignature(), errors);
        if (nb_flows > signature_flow_limit) {
            errors.emplace_back("Flow count of " + std::to_string(nb_flows)
                                + " exceeds signature flow limit of "
                                + std::to_string(signature_flow_limit)
                                + id_string(def_idx) + ".");
        }
    } else if (nb_flows > api_flow_limit) {
        errors.emplace_back("Flow count of " + std::to_string(nb_flows)
                            + " exceeds flow limit of "
                            + std::to_string(api_flow_limit)
                            + id_string(def_idx) + ".");
    }
}

static void validate(const duration_ptr& duration,
                     std::vector<std::string>& errors)
{
    bool have_config = false;
    if (duration->continuousIsSet() && duration->isContinuous()) {
        have_config = true;
    } else if (duration->framesIsSet()) {
        if (duration->getFrames() < 1) {
            errors.emplace_back("Duration frame limit must be positive.");
        }
        have_config = true;
    } else if (duration->timeIsSet()) {
        const auto& time = duration->getTime();
        if (time->getValue() < 1) {
            errors.emplace_back("Duration time value must be positive.");
        }
        if (to_period_type(time->getUnits()) == period_type::none) {
            errors.emplace_back("Duration time units, " + time->getUnits()
                                + ", are invalid.");
        }
        have_config = true;
    }

    if (!have_config) {
        errors.emplace_back("No duration configuration found.");
    }
}

static void validate(const load_ptr& load, std::vector<std::string>& errors)
{
    if (load->burstSizeIsSet() & (load->getBurstSize() < 1)) {
        errors.emplace_back("Load burst size must be positive.");
    }

    auto rate = load->getRate();
    if (!rate) {
        errors.emplace_back("Load rate is required.");
    } else {
        if (rate->getValue() < 1) {
            errors.emplace_back("Load rate value must be positive.");
        }

        if (to_period_type(rate->getPeriod()) == period_type::none) {
            errors.emplace_back("Load period, " + rate->getPeriod()
                                + ", is invalid.");
        }
    }

    if (to_load_type(load->getUnits()) == load_type::none) {
        errors.emplace_back("Load units, " + load->getUnits()
                            + ", are invalid.");
    }
}

static void validate(const generator_config_ptr& config,
                     std::vector<std::string>& errors)
{
    /* Verify traffic duration */
    const auto& duration = config->getDuration();
    if (!duration) {
        errors.emplace_back("Traffic duration is required.");
    } else {
        validate(duration, errors);
    }

    /* Verify traffic load */
    auto load = config->getLoad();
    if (!load) {
        errors.emplace_back("Traffic load is required.");
    } else {
        validate(load, errors);
    }

    /* Make sure order is valid if set */
    if (config->orderIsSet()) {
        if (to_order_type(config->getOrder()) == order_type::none) {
            errors.emplace_back("Order, " + config->getOrder()
                                + ", is invalid.");
        }
    } else if (config->getTraffic().size() > 1) {
        /*
         * Order must be defined when multiple traffic definitions
         * are present.
         */
        errors.emplace_back("Order must be set when multiple traffic "
                            "definitions are present.");
    }

    /* Verify traffic definitions */
    if (config->getTraffic().empty()) {
        errors.emplace_back("At least one traffic definition is required.");
    } else {
        auto idx = 1U;
        for (const auto& item : config->getTraffic()) {
            validate(item, idx++, errors);
        }
    }
}

bool is_valid(const generator_type& generator, std::vector<std::string>& errors)
{
    auto init_errors = errors.size();

    if (generator.getTargetId().empty()) {
        errors.emplace_back("Packet generator target id is required.");
    }

    auto config = generator.getConfig();
    if (!config) {
        errors.emplace_back("Packet generator configuration is required.");
        return (false);
    }

    assert(config);

    validate(config, errors);

    return (init_errors == errors.size());
}

} // namespace openperf::packet::generator::api
