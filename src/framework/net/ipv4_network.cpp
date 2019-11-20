#include <stdexcept>

#include "net/ipv4_network.hpp"

namespace openperf {
namespace net {

ipv4_network::ipv4_network(const ipv4_address& addr, uint8_t prefix)
    : m_prefix(prefix)
{
    if (m_prefix > 32) {
        throw std::out_of_range(std::to_string(m_prefix) + " is larger than 32");
    }
    ipv4_address netmask(static_cast<uint32_t>(~0) << (32 - m_prefix));
    m_addr = ipv4_address(addr.data() & netmask.data());
}

const ipv4_address& ipv4_network::address() const
{
    return (m_addr);
}

uint8_t ipv4_network::prefix_length() const
{
    return (m_prefix);
}

std::string to_string(const ipv4_network& network)
{
    return (to_string(network.address()) + "/" + std::to_string(network.prefix_length()));
}

int compare(const ipv4_network& lhs, const ipv4_network& rhs)
{
    if      (lhs.address()       < rhs.address()      ) return (-1);
    else if (lhs.address()       > rhs.address()      ) return (1);
    else if (lhs.prefix_length() < rhs.prefix_length()) return (-1);
    else if (lhs.prefix_length() > rhs.prefix_length()) return (1);
    else return (0);
}

}
}
