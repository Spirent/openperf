#include <cstdlib>

#include "packet/type/ipv6_network.hpp"

namespace libpacket::type {

static ipv6_address prefix_mask(uint8_t prefix_length)
{
    if (prefix_length > ipv6_network::max_prefix_length) {
        throw std::out_of_range(
            std::to_string(prefix_length) + " is larger than "
            + std::to_string(ipv6_network::max_prefix_length));
    }

    auto q = ldiv(prefix_length, 8);
    auto idx = 0U;
    auto prefix = ipv6_address{};

    while (idx < q.quot) { prefix[idx++] = 0xff; }
    if (q.rem) { prefix[idx] = 0xff << (8 - q.rem); }

    return (prefix);
}

ipv6_network::ipv6_network(const ipv6_address& addr, uint8_t prefix)
    : m_addr(addr & prefix_mask(prefix))
    , m_prefix(prefix)
{}

ipv6_address ipv6_network::address() const { return (m_addr); }

uint8_t ipv6_network::prefix_length() const { return (m_prefix); }

std::string to_string(const ipv6_network& network)
{
    return (to_string(network.address()) + "/"
            + std::to_string(network.prefix_length()));
}

int compare(const ipv6_network& lhs, const ipv6_network& rhs)
{
    if (lhs.address() < rhs.address()) {
        return (-1);
    } else if (lhs.address() > rhs.address()) {
        return (1);
    } else {
        if (lhs.prefix_length() < rhs.prefix_length()) {
            return (-1);
        } else if (lhs.prefix_length() > rhs.prefix_length()) {
            return (1);
        } else {
            return (0);
        }
    }
}

} // namespace libpacket::type
