#include "internal_utils.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <poll.h>

namespace openperf::network::internal {

static void* get_sa_addr(const sockaddr* sa)
{
    switch (sa->sa_family) {
    case AF_INET:
        return (&((sockaddr_in*)sa)->sin_addr);
    case AF_INET6:
        return (&((sockaddr_in6*)sa)->sin6_addr);
    default:
        return (NULL);
    }
}

socklen_t network_sockaddr_size(const network_sockaddr& s)
{
    switch (s.sa.sa_family) {
    case AF_INET:
        return (sizeof(s.sa4));
    case AF_INET6:
        return (sizeof(s.sa6));
    default:
        return 0;
    }
}

in_port_t network_sockaddr_port(const network_sockaddr& s)
{
    switch (s.sa.sa_family) {
    case AF_INET:
        return (s.sa4.sin_port);
    case AF_INET6:
        return (s.sa6.sin6_port);
    default:
        return 0;
    }
}

void* network_sockaddr_addr(const network_sockaddr& s)
{
    switch (s.sa.sa_family) {
    case AF_INET:
        return ((void*)&s.sa4.sin_addr);
    case AF_INET6:
        return ((void*)&s.sa6.sin6_addr);
    default:
        return nullptr;
    }
}

const char*
network_sockaddr_addr_str(const network_sockaddr& s, char* buf, size_t buf_len)
{
    void* addr = network_sockaddr_addr(s);
    if (!addr) {
        if (buf_len < 1) return 0;
        buf[0] = 0;
        return buf;
    }

    return inet_ntop(s.sa.sa_family, addr, buf, buf_len);
}

size_t network_addr_mask_to_prefix_len(const uint8_t* bytes, size_t n_bytes)
{
    for (size_t i = 0; i < n_bytes; ++i) {
        uint8_t val = bytes[n_bytes - i - 1];
        if (val == 0) continue;
        /* Find first bit in byte which is set */
        int bit_pos = __builtin_ffs((long)val) - 1;
        size_t pos = 8 * (n_bytes - i) - bit_pos;
        return pos;
    }
    return 0;
}

bool ipv6_addr_is_link_local(const in6_addr* addr)
{
    if (addr->s6_addr[0] == 0xfe && (addr->s6_addr[1] & 0xC0) == 0x80)
        return true;
    return false;
}

/**
 * Check if IPv6 address is an IPv4 mapped address.
 */
bool ipv6_addr_is_ipv4_mapped(const in6_addr* addr)
{
    uint16_t* words = (uint16_t*)addr->s6_addr;
    if (words[0] == 0 && words[1] == 0 && words[2] == 0 && words[3] == 0
        && words[4] == 0 && words[5] == 0xffff) {
        return true;
    }
    return false;
}

/**
 * Get IPv4 address represented by the IPv6 mapped address.
 */
void ipv6_get_ipv4_mapped(const in6_addr* v6, in_addr* v4)
{
    v4->s_addr = (v6->s6_addr[15] << 24) | (v6->s6_addr[14] << 16)
                 | (v6->s6_addr[13] << 8) | v6->s6_addr[12];
}

/**
 * Return the peer of the given socket in a freshly allocated string.
 */
char* get_peer_address(int socket)
{
    char* host = NULL;
    network_sockaddr peer;
    socklen_t peer_length = sizeof(peer);
    memset(&peer, 0, peer_length);

    if ((getpeername(socket, &peer.sa, &peer_length) != 0)
        || ((host = (char*)calloc(INET6_ADDRSTRLEN, sizeof(char))) == NULL)) {
        return (NULL);
    }

    if (peer.sa.sa_family == AF_INET6) {
        if (ipv6_addr_is_ipv4_mapped(&peer.sa6.sin6_addr)) {
            /* Return IPv4 address for mapped addresses */
            in_addr v4;
            ipv6_get_ipv4_mapped(&peer.sa6.sin6_addr, &v4);
            inet_ntop(AF_INET, &v4, host, INET6_ADDRSTRLEN);
            return host;
        }
    }

    inet_ntop(peer.sa.sa_family, get_sa_addr(&peer.sa), host, INET6_ADDRSTRLEN);

    return (host);
}

/**
 * Logs the name and IP address of all interfaces.
 */
void log_intf_addresses(void)
{
    ifaddrs *ifa = NULL, *ifa_free = NULL;

    char buffer[INET6_ADDRSTRLEN];
    void* address = NULL;
    memset(buffer, 0, INET6_ADDRSTRLEN);

    if (getifaddrs(&ifa_free) == 0) {
        for (ifa = ifa_free; ifa != NULL && ifa->ifa_addr != NULL;
             ifa = ifa->ifa_next) {
            if ((ifa->ifa_flags & IFF_LOOPBACK) == 0) {
                address = get_sa_addr(ifa->ifa_addr);
                if (address != NULL) {
                    inet_ntop(ifa->ifa_addr->sa_family,
                              address,
                              buffer,
                              INET6_ADDRSTRLEN);
                }
            }
        }
        freeifaddrs(ifa_free);
    }
}

/**
 * Get the prefix length for the mask.
 */
size_t addr_mask_to_prefix_len(const uint8_t* bytes, size_t n_bytes)
{
    for (size_t i = 0; i < n_bytes; ++i) {
        uint8_t val = bytes[n_bytes - i - 1];
        if (val == 0) continue;
        /* Find first bit in byte which is set */
        int bit_pos = __builtin_ffs((long)val) - 1;
        size_t pos = 8 * (n_bytes - i) - bit_pos;
        return pos;
    }
    return 0;
}

/**
 * Get the prefix length for the IPv4 mask.
 */
size_t ipv4_addr_mask_to_prefix_len(const in_addr* addr)
{
    return addr_mask_to_prefix_len((const uint8_t*)&addr->s_addr, 4);
}

/**
 * Get the prefix length for the IPv6 mask.
 */
size_t ipv6_addr_mask_to_prefix_len(const in6_addr* addr)
{
    return addr_mask_to_prefix_len(addr->s6_addr, 16);
}

/**
 * Send ICMPv6 echo request mssage
 */
static int ipv6_send_icmp_echo_req(int sock,
                                   int ifindex,
                                   const in6_addr* src_addr,
                                   const in6_addr* dst_addr,
                                   int id,
                                   int seq)
{
    msghdr msg;
    iovec iov;
    uint8_t req[sizeof(icmp6_hdr)];
    uint8_t req_ctrl[CMSG_SPACE(sizeof(in6_pktinfo))];
    sockaddr_in6 dst;

    memset(&iov, 0, sizeof(iov));
    iov.iov_base = req;
    iov.iov_len = 0;
    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    memset(&dst, 0, sizeof(dst));
    dst.sin6_family = AF_INET6;
    dst.sin6_addr = *dst_addr;
    msg.msg_name = (caddr_t)&dst;
    msg.msg_namelen = sizeof(dst);
    msg.msg_control = req_ctrl;
    msg.msg_controllen = sizeof(req_ctrl);

    cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    msg.msg_controllen = 0;
    if (ifindex >= 0) {
        dst.sin6_scope_id = ifindex;

        cmsg->cmsg_level = IPPROTO_IPV6;
        cmsg->cmsg_len = CMSG_LEN(sizeof(in6_pktinfo));
        cmsg->cmsg_type = IPV6_PKTINFO;

        in6_pktinfo* pi = (in6_pktinfo*)CMSG_DATA(cmsg);
        pi->ipi6_ifindex = ifindex;
        pi->ipi6_addr = *src_addr;
        msg.msg_controllen += CMSG_SPACE(sizeof(in6_pktinfo));
    }

    icmp6_hdr* icmp6 = (icmp6_hdr*)req;
    icmp6->icmp6_type = ICMP6_ECHO_REQUEST;
    icmp6->icmp6_code = 0;
    icmp6->icmp6_cksum = 0;
    icmp6->icmp6_id = htons(id);
    icmp6->icmp6_seq = htons(seq);
    iov.iov_len += 8;

    int len = sendmsg(sock, &msg, 0);
    if (len < 0) {
        OP_LOG(OP_LOG_ERROR,
               "%s: Failed sending ICMPv6 echo request ifindex=%d.  %s\n",
               __FUNCTION__,
               ifindex,
               strerror(errno));
    }
    return (len == (ssize_t)iov.iov_len) ? 0 : -1;
}

/**
 * Extract ifindex from received msg info.
 */
static int ipv6_get_recv_msg_ifindex(msghdr* msg)
{
    for (cmsghdr* cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL;
         cmsg = CMSG_NXTHDR(msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IPV6
            && cmsg->cmsg_type == IPV6_PKTINFO) {
            in6_pktinfo* pktinfo = (in6_pktinfo*)CMSG_DATA(cmsg);
            return pktinfo->ipi6_ifindex;
        }
    }
    return -1;
}

/**
 * Get neighbor ifindex from ICMPv6 ping by using a socket.
 */
int ipv6_get_neighbor_ifindex_from_ping_socket(const in6_addr* addr)
{
    const int max_retry = 2;
    const int wait_timeout = 2000; /* msec */
    ifaddrs* ifaddrs = NULL;
    int sock;
    int enable = 1;
    int ifindex = -1;
    uint8_t resp_control[CMSG_SPACE(sizeof(in6_pktinfo))];
    bool link_local = ipv6_addr_is_link_local(addr);
    uint8_t resp[4096];

    sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    if (sock < 0) { return -1; }

    if (link_local) {
        if (getifaddrs(&ifaddrs) != 0) {
            close(sock);
            return -1;
        }
    }

    if (setsockopt(
            sock, IPPROTO_IPV6, IPV6_RECVPKTINFO, &enable, sizeof(enable))
        < 0) {
        OP_LOG(OP_LOG_ERROR,
               "%s: Failed enabling IPV6_RECVPKTINFO\n",
               __FUNCTION__);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable))
        < 0) {
        OP_LOG(OP_LOG_ERROR, "%s: Failed setting SO_BROADCAST\n", __FUNCTION__);
    }
    int chksum_offset = offsetof(icmp6_hdr, icmp6_cksum);
    if (setsockopt(
            sock, SOL_RAW, IPV6_CHECKSUM, &chksum_offset, sizeof(chksum_offset))
        < 0) {
        // This isn't required for OSv so don't print error message
        // OP_LOG(OP_LOG_ERROR, "%s: Failed setting IPV6_CHECKSUM\n",
        // __FUNCTION__);
    }

    for (int retry = 0; retry < max_retry; ++retry) {
        int echo_req_count = 0;

        if (link_local) {
            /* Send ECHO request out all interfaces */
            for (struct ifaddrs* ifa = ifaddrs; ifa != NULL;
                 ifa = ifa->ifa_next) {
                if (ifa->ifa_flags & IFF_LOOPBACK) continue;
                if (ifa->ifa_addr->sa_family == AF_INET6) {
                    sockaddr_in6* src_addr = (sockaddr_in6*)ifa->ifa_addr;
                    if (!ipv6_addr_is_link_local(&src_addr->sin6_addr))
                        continue;

                    int ifa_ifindex = if_nametoindex(ifa->ifa_name);
                    if (ifa_ifindex < 0) {
                        OP_LOG(OP_LOG_ERROR,
                               "%s: Failed getting ifindex for interface %s\n",
                               __FUNCTION__,
                               ifa->ifa_name);
                        continue;
                    }
                    if (ipv6_send_icmp_echo_req(sock,
                                                ifa_ifindex,
                                                &src_addr->sin6_addr,
                                                addr,
                                                0,
                                                retry + 1)
                        == 0) {
                        ++echo_req_count;
                    }
                }
            }
        } else {
            if (ipv6_send_icmp_echo_req(sock, -1, NULL, addr, 0, retry + 1)
                == 0) {
                ++echo_req_count;
            }
        }

        /* Wait for ECHO response */
        if (echo_req_count) {
            pollfd pfd[1];
            timeval tv;
            msghdr msg;
            iovec iov;
            sockaddr_in6 src_addr;
            uint64_t now, end_time;
            int resp_len;

            gettimeofday(&tv, NULL);
            now = tv.tv_sec * 1000000 + tv.tv_usec;
            end_time = now + wait_timeout * 1000;

            while (1) {
                gettimeofday(&tv, NULL);
                now = tv.tv_sec * 1000000 + tv.tv_usec;
                if (now > end_time) break;

                pfd[0].fd = sock;
                pfd[0].events = POLLIN;
                pfd[0].revents = 0;

                if (poll(pfd, 1, (end_time - now) / 1000) == 0) break;

                memset(&msg, 0, sizeof(msg));
                memset(&iov, 0, sizeof(iov));
                iov.iov_base = resp;
                iov.iov_len = sizeof(resp);
                msg.msg_iov = &iov;
                msg.msg_iovlen = 1;
                msg.msg_control = resp_control;
                msg.msg_controllen = sizeof(resp_control);
                msg.msg_name = &src_addr;
                msg.msg_namelen = sizeof(src_addr);

                while ((resp_len = recvmsg(sock, &msg, MSG_DONTWAIT)) > 0) {
                    icmp6_hdr* icmp6 = (icmp6_hdr*)resp;
                    if (icmp6->icmp6_type != ICMP6_ECHO_REPLY) continue;
                    if (src_addr.sin6_family == AF_INET6
                        && memcmp(addr, &src_addr.sin6_addr, sizeof(*addr))
                               == 0) {
                        if ((ifindex = ipv6_get_recv_msg_ifindex(&msg)) >= 0)
                            break;
                    }
                }
                if (ifindex >= 0) break;
            }
        }
    }

    if (ifaddrs) freeifaddrs(ifaddrs);

    close(sock);

    return ifindex;
}

} // namespace openperf::network::internal