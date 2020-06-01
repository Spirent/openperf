#ifndef _LIB_PACKET_TYPE_IPV6_NETWORK_HPP_
#define _LIB_PACKET_TYPE_IPV6_NETWORK_HPP_

#include <string>

#include "packet/type/ipv6_address.hpp"

namespace libpacket::type {

/**
 * Immutable Ipv6 Network
 */

class ipv6_network
{
    ipv6_address m_addr;
    uint8_t m_prefix;

public:
    ipv6_network(const ipv6_address&, uint8_t);

    ipv6_address address() const;
    uint8_t prefix_length() const;

    constexpr static uint8_t max_prefix_length = 128;
};

std::string to_string(const ipv6_network&);

int compare(const ipv6_network&, const ipv6_network&);

inline bool operator==(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) == 0;
}

inline bool operator!=(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) != 0;
}

inline bool operator<(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) < 0;
}

inline bool operator>(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) > 0;
}

inline bool operator<=(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) <= 0;
}

inline bool operator>=(const ipv6_network& lhs, const ipv6_network& rhs)
{
    return compare(lhs, rhs) >= 0;
}

} // namespace libpacket::type

#endif /* _LIB_PACKET_TYPE_IPV6_NETWORK_HPP_ */
