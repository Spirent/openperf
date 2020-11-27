#ifndef _OP_NETWORK_INTERNAL_UTILS_HPP_
#define _OP_NETWORK_INTERNAL_UTILS_HPP_

#include <arpa/inet.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <poll.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cassert>

#include "framework/core/op_log.h"

namespace openperf::network::internal {

union network_sockaddr
{
    sockaddr sa;
    sockaddr_in sa4;
    sockaddr_in6 sa6;
};

socklen_t network_sockaddr_size(const network_sockaddr& s);

in_port_t network_sockaddr_port(const network_sockaddr& s);

void* network_sockaddr_addr(const network_sockaddr& s);

const char*
network_sockaddr_addr_str(const network_sockaddr& s, char* buf, size_t buf_len);

size_t network_addr_mask_to_prefix_len(const uint8_t* bytes, size_t n_bytes);

bool ipv6_addr_is_link_local(const in6_addr* addr);

bool ipv6_addr_is_ipv4_mapped(const in6_addr* addr);

void ipv6_get_ipv4_mapped(const in6_addr* v6, in_addr* v4);

char* get_mac_address(const char* ifname);

char* get_peer_address(int socket);

void log_intf_addresses(void);

size_t addr_mask_to_prefix_len(const uint8_t* bytes, size_t n_bytes);

size_t ipv4_addr_mask_to_prefix_len(const in_addr* addr);

size_t ipv6_addr_mask_to_prefix_len(const in6_addr* addr);

bool ipv6_link_local_requires_scope();

int ipv6_get_neighbor_ifindex(const in6_addr* addr);

/* These shouldn't be used directly.  Need to expose them for unit tests. */
int ipv6_get_neighbor_ifindex_from_cache(const in6_addr* addr);

int ipv6_get_neighbor_ifindex_from_ping(const in6_addr* addr);

int ipv6_get_neighbor_ifindex_from_ping_socket(const in6_addr* addr);

} // namespace openperf::network::internal

#endif
