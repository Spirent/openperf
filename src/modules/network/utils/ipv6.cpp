#include "ipv6.hpp"

#include <cstdlib>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/icmp6.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <cerrno>
#include <poll.h>

#include "core/op_log.h"

namespace openperf::network::internal {

bool ipv6_addr_is_link_local(const in6_addr* addr)
{
    if (addr->s6_addr[0] == 0xfe && (addr->s6_addr[1] & 0xC0) == 0x80)
        return true;
    return false;
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

        auto pi = reinterpret_cast<in6_pktinfo*>(CMSG_DATA(cmsg));
        pi->ipi6_ifindex = ifindex;
        pi->ipi6_addr = *src_addr;
        msg.msg_controllen += CMSG_SPACE(sizeof(in6_pktinfo));
    }

    auto icmp6 = reinterpret_cast<icmp6_hdr*>(req);
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
    for (cmsghdr* cmsg = CMSG_FIRSTHDR(msg); cmsg != nullptr;
         cmsg = CMSG_NXTHDR(msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IPV6
            && cmsg->cmsg_type == IPV6_PKTINFO) {
            auto pktinfo = reinterpret_cast<in6_pktinfo*>(CMSG_DATA(cmsg));
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
    ifaddrs* ifaddrs = nullptr;
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
            for (auto ifa = ifaddrs; ifa != nullptr; ifa = ifa->ifa_next) {
                if (ifa->ifa_flags & IFF_LOOPBACK) continue;
                if (ifa->ifa_addr->sa_family == AF_INET6) {
                    auto src_addr =
                        reinterpret_cast<sockaddr_in6*>(ifa->ifa_addr);
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
            if (ipv6_send_icmp_echo_req(sock, -1, nullptr, addr, 0, retry + 1)
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

            gettimeofday(&tv, nullptr);
            now = tv.tv_sec * 1000000 + tv.tv_usec;
            end_time = now + wait_timeout * 1000;

            while (true) {
                gettimeofday(&tv, nullptr);
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
                    auto icmp6 = reinterpret_cast<icmp6_hdr*>(resp);
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
