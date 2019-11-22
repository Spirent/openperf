#ifndef _OP_FRAMEWORK_NET_IPV4_NETWORK_HPP_
#define _OP_FRAMEWORK_NET_IPV4_NETWORK_HPP_

#include "net/ipv4_address.hpp"

namespace openperf {
namespace net {

class ipv4_network
{
public:
    ipv4_network(const ipv4_address&, uint8_t);

    const ipv4_address& address() const;
    uint8_t prefix_length() const;

private:
    ipv4_address m_addr;
    uint8_t m_prefix;
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

} // namespace net
} // namespace openperf

#endif /* _OP_FRAMEWORK_NET_IPV4_NETWORK_HPP_ */
