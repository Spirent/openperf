#ifndef _LIB_PACKET_TYPE_IPV4_NETWORK_HPP_
#define _LIB_PACKET_TYPE_IPV4_NETWORK_HPP_

#include <string>

#include "packet/type/ipv4_address.hpp"

namespace libpacket::type {

/**
 * Immutable IPv4 Network
 */

class ipv4_network
{
    ipv4_address m_addr;
    uint8_t m_prefix;

public:
    ipv4_network(const ipv4_address&, uint8_t);

    ipv4_address address() const;
    uint8_t prefix_length() const;

    constexpr static uint8_t max_prefix_length = 32;
};

std::string to_string(const ipv4_network&);

int compare(const ipv4_network&, const ipv4_network&);

inline bool operator==(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) == 0;
}

inline bool operator!=(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) != 0;
}

inline bool operator<(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) < 0;
}

inline bool operator>(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) > 0;
}

inline bool operator<=(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) <= 0;
}

inline bool operator>=(const ipv4_network& lhs, const ipv4_network& rhs)
{
    return compare(lhs, rhs) >= 0;
}

} // namespace libpacket::type

#endif /* _LIB_PACKET_TYPE_IPV4_NETWORK_HPP_ */
