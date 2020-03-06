#include <stdexcept>

#include "net/ipv6_network.hpp"

namespace openperf {
namespace net {

ipv6_network::ipv6_network(const ipv6_address& addr, uint8_t prefix)
    : m_prefix(prefix)
{
    ipv6_address netmask(ipv6_address::make_prefix_mask(m_prefix));
    m_addr = addr & netmask;
}

const ipv6_address& ipv6_network::address() const { return (m_addr); }

uint8_t ipv6_network::prefix_length() const { return (m_prefix); }

std::string to_string(const ipv6_network& network)
{
    return (to_string(network.address()) + "/"
            + std::to_string(network.prefix_length()));
}

int compare(const ipv6_network& lhs, const ipv6_network& rhs)
{
    if (lhs.address() < rhs.address())
        return (-1);
    else if (lhs.address() > rhs.address())
        return (1);
    else if (lhs.prefix_length() < rhs.prefix_length())
        return (-1);
    else if (lhs.prefix_length() > rhs.prefix_length())
        return (1);
    else
        return (0);
}

} // namespace net
} // namespace openperf
