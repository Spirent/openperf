#ifndef _OP_NETWORK_UTILS_IPV6_HPP_
#define _OP_NETWORK_UTILS_IPV6_HPP_

#include <arpa/inet.h>

namespace openperf::network::internal {

int ipv6_get_neighbor_ifindex(const in6_addr* addr);

} // namespace openperf::network::internal

#endif
