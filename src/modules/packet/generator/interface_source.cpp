#include "packet/generator/interface_source.hpp"

#include "lib/packet/type/ipv4_address.hpp"
#include "lib/packet/type/mac_address.hpp"

#include "core/op_log.h"

namespace openperf::packet::generator {

interface_source::interface_source(core::event_loop& loop,
                                   packetio::interface::generic_interface& intf)
    : m_learning(loop, intf)
    , m_interface(intf)
{}

bool same_ipv4_subnet(const libpacket::type::ipv4_address& addr1,
                      const libpacket::type::ipv4_address& addr2,
                      const libpacket::type::ipv4_address& netmask)
{
    return ((addr1 & netmask) == (addr2 & netmask));
}

using ipv4_address_tuple =
    std::tuple<libpacket::type::ipv4_address, // Interface Address
               libpacket::type::ipv4_address, // Gateway Address
               libpacket::type::ipv4_address, // Netmask
               bool // true if three address above were found and valid. false
                    // otherwise.
               >;

static constexpr ipv4_address_tuple ipv4_addresses_not_found =
    ipv4_address_tuple(libpacket::type::ipv4_address(),
                       libpacket::type::ipv4_address(),
                       libpacket::type::ipv4_address(),
                       false);

ipv4_address_tuple
get_ipv4_interface_addresses(const packetio::interface::generic_interface& intf)
{
    auto maybe_addr_str = intf.ipv4_address();
    if (!maybe_addr_str.has_value()) { return (ipv4_addresses_not_found); }

    auto intf_addr = libpacket::type::ipv4_address(maybe_addr_str.value());

    auto maybe_gateway_str = intf.ipv4_gateway();
    if (!maybe_gateway_str.has_value()) { return (ipv4_addresses_not_found); }

    auto gateway_addr =
        libpacket::type::ipv4_address(maybe_gateway_str.value());

    auto maybe_netmask_int = intf.ipv4_prefix_length();
    if (!maybe_netmask_int.has_value()) { return (ipv4_addresses_not_found); }

    auto netmask = libpacket::type::ipv4_address(
        static_cast<uint32_t>(~0) << (32 - maybe_netmask_int.value()));

    return std::make_tuple(intf_addr, gateway_addr, netmask, true);
}

std::unordered_set<libpacket::type::ipv4_address> extract_addresses_to_learn(
    const traffic::sequence& sequence,
    const packetio::interface::generic_interface& interface)
{
    auto [intf_ipv4_addr,
          intf_ipv4_gateway_addr,
          intf_ipv4_netmask,
          intf_ipv4_valid] = get_ipv4_interface_addresses(interface);

    std::unordered_set<libpacket::type::ipv4_address> addresses_to_learn;

    // XXX: In an ideal world this would be a std::for_each with a lambda.
    // But lambda capture doesn't play well with structured binding under clang.
    // https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
    for (const auto& seq : sequence) {

        const auto hdr_ptr =
            std::get<traffic::sequence::pointer_to_header>(seq);
        const auto pkt_flags = std::get<traffic::sequence::protocol_flags>(seq);
        const auto hdr_lens = std::get<traffic::sequence::header_length>(seq);

        // Does the template have an IPv4 header?
        if (pkt_flags & packetio::packet::packet_type::ip::ipv4) {
            const auto* ipv4 =
                reinterpret_cast<const libpacket::protocol::ipv4*>(
                    hdr_ptr + hdr_lens.layer2);

            auto dest_ip = get_ipv4_destination(*ipv4);

            if (same_ipv4_subnet(
                    intf_ipv4_gateway_addr, dest_ip, intf_ipv4_netmask)) {
                addresses_to_learn.insert(dest_ip);
            } else {
                addresses_to_learn.insert(intf_ipv4_gateway_addr);
            }
        }

        // XXX: when IPv6 support gets added there will be a block
        // here to extract those addresses.
    }

    return (addresses_to_learn);
}

void set_ipv4_dest_mac(const learning_result_map& learning_results,
                       const libpacket::type::ipv4_address& ipv4_dest,
                       libpacket::protocol::ethernet* eth_hdr)
{
    auto maybe_result_entry = learning_results.find(ipv4_dest);
    if (maybe_result_entry == learning_results.end()) {
        OP_LOG(OP_LOG_ERROR,
               "Unable to find resolved MAC entry for IP: %s",
               to_string(ipv4_dest).c_str());
        return;
    }

    if (std::holds_alternative<unresolved>(maybe_result_entry->second)) {
        OP_LOG(OP_LOG_ERROR,
               "MAC for IP %s is still unresolved.",
               to_string(ipv4_dest).c_str());
        return;
    }

    auto mac =
        std::get<libpacket::type::mac_address>(maybe_result_entry->second);

    set_ethernet_destination(*eth_hdr, mac);
}

static void
process_learning_results(const learning_state_machine& lsm,
                         traffic::sequence& sequence,
                         packetio::interface::generic_interface& interface)
{
    // This function is called iff all addresses resolved correctly.

    auto [intf_ipv4_addr,
          intf_ipv4_gateway_addr,
          intf_ipv4_netmask,
          intf_ipv4_valid] = get_ipv4_interface_addresses(interface);

    const auto& learning_results = lsm.results();

    // XXX: In an ideal world this would be a std::for_each with a lambda.
    // But lambda capture doesn't play well with structured binding under clang.
    // https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
    for (auto seq : sequence) {

        auto hdr_ptr = std::get<traffic::sequence::pointer_to_header>(seq);
        const auto pkt_flags = std::get<traffic::sequence::protocol_flags>(seq);
        const auto hdr_lens = std::get<traffic::sequence::header_length>(seq);

        auto* eth = reinterpret_cast<libpacket::protocol::ethernet*>(
            const_cast<uint8_t*>(hdr_ptr));

        // Does the template have an IPv4 header?
        if (pkt_flags & packetio::packet::packet_type::ip::ipv4) {
            if (!intf_ipv4_valid) { continue; }

            auto* ipv4 = const_cast<libpacket::protocol::ipv4*>(
                reinterpret_cast<const libpacket::protocol::ipv4*>(
                    hdr_ptr + hdr_lens.layer2));

            auto ip_addr = get_ipv4_destination(*ipv4);

            if (same_ipv4_subnet(
                    intf_ipv4_gateway_addr, ip_addr, intf_ipv4_netmask)) {

                set_ipv4_dest_mac(learning_results, ip_addr, eth);
            } else {
                set_ipv4_dest_mac(
                    learning_results, intf_ipv4_gateway_addr, eth);
            }
        }

        // XXX: when IPv6 support gets added there will be a block
        // here to update those addresses.
    }
}

bool interface_source::start_learning(traffic::sequence& sequence)
{
    auto to_learn = extract_addresses_to_learn(sequence, m_interface);

    if (to_learn.empty()) { return (false); }

    auto callback = [&](const learning_state_machine& lsm) {
        process_learning_results(lsm, sequence, m_interface);
    };

    m_learning.start_learning(to_learn, callback);

    return (true);
}

void interface_source::stop_learning() { m_learning.stop_learning(); }

bool interface_source::retry_learning()
{
    return (m_learning.retry_learning());
}

// XXX: In an ideal world this would be constexpr.
// But, the underlying std::array type doesn't zero out POD values.
// So there's no guarantee that the IP would end up as "0.0.0.0" for all
// platforms/compilers.
static const libpacket::type::ipv4_address unspecified_ipv4 =
    libpacket::type::ipv4_address("0.0.0.0");

// XXX: see above comment for unspecified_ipv4.
static const libpacket::type::mac_address unspecified_mac =
    libpacket::type::mac_address("00:00:00:00:00:00");

void interface_source::populate_source_addresses(traffic::sequence& sequence)
{
    auto [intf_ipv4_addr,
          intf_ipv4_gateway_addr,
          intf_ipv4_netmask,
          intf_ipv4_valid] = get_ipv4_interface_addresses(m_interface);

    auto intf_mac_address =
        libpacket::type::mac_address(m_interface.mac_address());

    // XXX: In an ideal world this would be a std::for_each with a lambda.
    // But lambda capture doesn't play well with structured binding under
    // clang.
    // https://stackoverflow.com/questions/46114214/lambda-implicit-capture-fails-with-variable-declared-from-structured-binding
    for (auto seq : sequence) {
        auto hdr_ptr = std::get<traffic::sequence::pointer_to_header>(seq);
        const auto pkt_flags = std::get<traffic::sequence::protocol_flags>(seq);
        const auto hdr_lens = std::get<traffic::sequence::header_length>(seq);

        // Assume every template has at least an ethernet header.
        auto* eth = reinterpret_cast<libpacket::protocol::ethernet*>(
            const_cast<uint8_t*>(hdr_ptr));

        if (get_ethernet_source(*eth) == unspecified_mac) {
            // update source MAC iff user did not set one.
            set_ethernet_source(*eth, intf_mac_address);
        }

        // Does the template have an IPv4 header?
        if (pkt_flags & packetio::packet::packet_type::ip::ipv4) {
            if (!intf_ipv4_valid) { continue; }

            auto* ipv4 = const_cast<libpacket::protocol::ipv4*>(
                reinterpret_cast<const libpacket::protocol::ipv4*>(
                    hdr_ptr + hdr_lens.layer2));

            if (get_ipv4_source(*ipv4) == unspecified_ipv4) {
                // update source IP iff user did not set one.
                set_ipv4_source(*ipv4, intf_ipv4_addr);
            }
        }

        // XXX: when IPv6 support gets added there will be a block
        // here to update those addresses.
    }
}
learning_resolved_state interface_source::learning_state() const
{
    return (m_learning.state());
}

std::string interface_source::target_port() const
{
    return (m_interface.port_id());
}

} // namespace openperf::packet::generator