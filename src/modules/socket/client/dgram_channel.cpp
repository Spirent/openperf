#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <numeric>
#include <unistd.h>
#include <sys/eventfd.h>

#include "socket/client/dgram_channel.h"

namespace icp {
namespace socket {
namespace client {

static std::optional<dgram_channel_addr> to_addr(const sockaddr *addr)
{
    if (!addr) return (std::nullopt);

    switch (addr->sa_family) {
    case AF_INET: {
        auto sa4 = reinterpret_cast<const sockaddr_in*>(addr);
        return (std::make_optional<dgram_channel_addr>(sa4->sin_addr.s_addr,
                                                       ntohs(sa4->sin_port)));
    }
    case AF_INET6: {
        auto sa6 = reinterpret_cast<const sockaddr_in6*>(addr);
        return (std::make_optional<dgram_channel_addr>(
                    reinterpret_cast<const uint32_t*>(&sa6->sin6_addr.s6_addr),
                    ntohs(sa6->sin6_port)));
    }
    default:
        return (std::nullopt);
    }
}

static std::optional<sockaddr_storage> to_sockaddr(const dgram_channel_addr& addr)
{
    struct sockaddr_storage sstorage;

    switch (addr.addr().type) {
    case dgram_ip_addr::type::IPv4: {
        auto sa4 = reinterpret_cast<sockaddr_in*>(&sstorage);
        sa4->sin_family = AF_INET;
        sa4->sin_port = addr.port();
        sa4->sin_addr.s_addr = addr.addr().u_addr.ip4.addr;
        return (std::make_optional(sstorage));
    }
    case dgram_ip_addr::type::IPv6: {
        auto sa6 = reinterpret_cast<sockaddr_in6*>(&sstorage);
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = addr.port();
        std::memcpy(&sa6->sin6_addr.s6_addr, &addr.addr().u_addr.ip6.addr[0],
                    sizeof(in6_addr));
        return (std::make_optional(sstorage));
    }
    default:
        return (std::nullopt);
    }
}

static socklen_t length_of(const sockaddr_storage& sstorage)
{
    return (sstorage.ss_family == AF_INET
            ? sizeof(sockaddr_in)
            : sizeof(sockaddr_in6));
}

dgram_channel::dgram_channel(int client_fd, int server_fd)
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

dgram_channel::~dgram_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);
}

tl::expected<size_t, int> dgram_channel::send(pid_t pid __attribute__((unused)),
                                              const iovec iov[], size_t iovcnt,
                                              const sockaddr *to)
{
    if (iovcnt + (to ? 1 : 0) > sendq.capacity()) return (ENOBUFS);

    auto dst = to_addr(to);
    if (to && !dst) return (tl::make_unexpected(EINVAL));

    bool empty = sendq.empty();

    for (size_t i = 0; i < iovcnt; i++) {
        sendq.push(dgram_channel_buf(iov[i].iov_base,
                                  iov[i].iov_len,
                                  dst ? true : i < iovcnt - 1));
    }
    if (dst) {
        sendq.push(*dst);
    }

    if (empty) {
        eventfd_write(client_fds.server_fd, 1);
    }

    return (std::accumulate(iov, iov + iovcnt, 0UL,
                            [](size_t x, const iovec &iov) -> size_t {
                                return (x + iov.iov_len);
                            }));
}

tl::expected<size_t, int> dgram_channel::recv(pid_t pid, iovec iov[], size_t iovcnt,
                                              sockaddr *from, socklen_t *fromlen)
{
    bool full = recvq.full();

    size_t nb_items;
    size_t item_len = 0;
    std::array<iovec, api::socket_queue_length> items;

    while (auto item = recvq.pop()) {
        std::visit(overloaded_visitor(
                       [&](dgram_channel_buf& buf) {
                           items[nb_items++] = iovec{
                               .iov_base = const_cast<void*>(buf.payload()),
                               .iov_len = buf.length()
                           };
                           if (!freeq.push(buf.pbuf())) {
                               fprintf(stderr, "Could not return pbuf %p to stack\n",
                                       reinterpret_cast<const void*>(buf.pbuf()));
                           }
                       },
                       [&](dgram_channel_addr& addr) {
                           auto src = to_sockaddr(addr);
                           if (src && from && fromlen) {
                               auto srclen = length_of(*src);
                               std::memcpy(from, &*src, std::min(*fromlen, srclen));
                               *fromlen = std::max(*fromlen, srclen);
                           }
                       }),
                   *item);
        if (!more(*item) || nb_items == api::socket_queue_length) break;
    }

    if (nb_items && full) {
        eventfd_write(client_fds.server_fd, 1);
    }

    return (process_vm_readv(pid, iov, iovcnt, items.data(), nb_items, 0));
}

int dgram_channel::recv_clear()
{
    uint64_t counter = 0;
    return (eventfd_read(client_fds.client_fd, &counter));
}

}
}
}
