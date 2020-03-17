#ifndef _OP_FRAMEWORK_NET_IPV6_NETWORK_HPP_
#define _OP_FRAMEWORK_NET_IPV6_NETWORK_HPP_

#include "net/ipv6_address.hpp"

namespace openperf {
namespace net {

/**
 * Immutable IPv6 Network
 */
class ipv6_network
{
public:
    ipv6_network(const ipv6_address&, uint8_t);

    const ipv6_address& address() const;
    uint8_t prefix_length() const;

private:
    ipv6_address m_addr;
    uint8_t m_prefix;
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

} // namespace net
} // namespace openperf

#endif /* _OP_FRAMEWORK_NET_IPV6_NETWORK_HPP_ */
