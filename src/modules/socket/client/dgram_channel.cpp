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
    : recvq(dgram_ring::client())
    , sendq(dgram_ring::client())
{
    client_fds.client_fd = client_fd;
    client_fds.server_fd = server_fd;
}

dgram_channel::~dgram_channel()
{
    close(client_fds.client_fd);
    close(client_fds.server_fd);
}

int dgram_channel::flags()
{
    return (socket_flags.load(std::memory_order_acquire));
}

int dgram_channel::flags(int flags)
{
    socket_flags.store(flags, std::memory_order_release);
    return (0);
}

tl::expected<size_t, int> dgram_channel::send(pid_t pid,
                                              const iovec iov[], size_t iovcnt,
                                              const sockaddr *to)
{
    if (!sendq.available()) return (ENOBUFS);

    bool empty = !sendq.ready();

    auto item = sendq.unpack();
    assert(item);
    item->address = to_addr(to);
    if (to && !item->address) return (tl::make_unexpected(EINVAL));

    iovec writevec = iovec{ .iov_base = const_cast<void*>(item->pvec.payload()),
                            .iov_len = item->pvec.len() };
    auto result = process_vm_writev(pid, iov, iovcnt, &writevec, 1, 0);
    if (result == -1) {
        return (tl::make_unexpected(errno));
    }

    item->pvec.len(result);
    sendq.repack();

    if (empty) {
        eventfd_write(client_fds.server_fd, 1);
    }

    return (result);
}

tl::expected<size_t, int> dgram_channel::recv(pid_t pid, iovec iov[], size_t iovcnt,
                                              sockaddr *from, socklen_t *fromlen)
{
    while (!recvq.available()) {
        uint64_t counter = 0;
        auto error = eventfd_read(client_fds.client_fd, &counter);
        if (error == -1) return (tl::make_unexpected(errno));
    }

    bool full = recvq.full();

    auto item = recvq.unpack();

    if (item->address) {
        auto src = to_sockaddr(*item->address);
        if (src && from && fromlen) {
            auto srclen = length_of(*src);
            std::memcpy(from, &*src, std::min(*fromlen, srclen));
            *fromlen = std::max(*fromlen, srclen);
        }
    }

    auto readvec = iovec { .iov_base = const_cast<void*>(item->pvec.payload()),
                           .iov_len = item->pvec.len() };
    auto result = process_vm_readv(pid, iov, iovcnt, &readvec, 1, 0);
    if (result == -1) {
        return (tl::make_unexpected(errno));
    }

    recvq.repack();

    if (full) {
        eventfd_write(client_fds.server_fd, 1);
    }

    return (result);
}

int dgram_channel::recv_clear()
{
    uint64_t counter = 0;
    return (eventfd_read(client_fds.client_fd, &counter));
}

}
}
}
