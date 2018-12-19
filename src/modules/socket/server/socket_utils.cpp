#include "socket/server/socket_utils.h"

namespace icp {
namespace socket {
namespace server {

std::optional<ip_addr_t> get_address(const sockaddr_storage& sstorage)
{
    switch (sstorage.ss_family) {
    case AF_INET: {
        auto sa4 = reinterpret_cast<const sockaddr_in*>(&sstorage);
        auto data = reinterpret_cast<const uint32_t*>(&sa4->sin_addr);
        return (std::make_optional<ip_addr_t>(IPADDR4_INIT(*data)));
    }
    case AF_INET6: {
        auto sa6 = reinterpret_cast<const sockaddr_in6*>(&sstorage);
        auto data = reinterpret_cast<const uint32_t*>(&sa6->sin6_addr);
        return (std::make_optional<ip_addr_t>(IPADDR6_INIT(data[0], data[1], data[2], data[3])));
        break;
    }
    case AF_UNSPEC:
        return (std::make_optional<ip_addr_t>(IPADDR_ANY_TYPE_INIT));
    default:
        return (std::nullopt);
    }
}

/**
 * lwIP wants port in host byte format.  Since clients expect sockaddr's to
 * store things in net byte format, we swap them here.
 */
std::optional<in_port_t> get_port(const sockaddr_storage& sstorage)
{
    switch (sstorage.ss_family) {
    case AF_INET: {
        auto sa4 = reinterpret_cast<const sockaddr_in*>(&sstorage);
        return (std::make_optional(ntohs(sa4->sin_port)));
    }
    case AF_INET6: {
        auto sa6 = reinterpret_cast<const sockaddr_in6*>(&sstorage);
        return (std::make_optional(ntohs(sa6->sin6_port)));
    }
    case AF_UNSPEC:
        return (std::make_optional(0));
    default:
        return (std::nullopt);
    }
}

tl::expected<void, int> copy_in(struct sockaddr_storage& dst,
                                pid_t src_pid, const sockaddr* src_ptr,
                                socklen_t length)
{
    constexpr socklen_t dst_length = sizeof(dst);
    if (length > dst_length) {
        return (tl::make_unexpected(ENAMETOOLONG));
    }

    auto local = iovec{
        .iov_base = &dst,
        .iov_len = dst_length
    };

    auto remote = iovec{
        .iov_base = const_cast<sockaddr*>(src_ptr),
        .iov_len = length
    };

    auto size = process_vm_readv(src_pid, &local, 1, &remote, 1, 0);
    if (size == -1) {
        return (tl::make_unexpected(errno));
    }

    return {};
}

tl::expected<int, int> copy_in(pid_t src_pid, const int *src_int)
{
    int value = 0;

    auto local = iovec{
        .iov_base = &value,
        .iov_len = sizeof(int)
    };

    auto remote = iovec{
        .iov_base = const_cast<int*>(src_int),
        .iov_len = sizeof(int)
    };

    auto size = process_vm_readv(src_pid, &local, 1, &remote, 1, 0);
    if (size == -1) {
        return (tl::make_unexpected(errno));
    }

    return (value);
}

tl::expected<void, int> copy_out(pid_t dst_pid, sockaddr* dst_ptr,
                                 const struct sockaddr_storage& src,
                                 socklen_t length)
{
    auto local = iovec{
        .iov_base = const_cast<sockaddr_storage*>(&src),
        .iov_len = length
    };

    auto remote = iovec {
        .iov_base = dst_ptr,
        .iov_len = length
    };

    auto size = process_vm_writev(dst_pid, &local, 1, &remote, 1, 0);
    if (size == -1) {
        return (tl::make_unexpected(errno));
    }

    return {};
}

tl::expected<void, int> copy_out(pid_t dst_pid, void* dst_ptr, int src)
{
    auto local = iovec{
        .iov_base = reinterpret_cast<void*>(&src),
        .iov_len = sizeof(int)
    };

    auto remote = iovec {
        .iov_base = dst_ptr,
        .iov_len = sizeof(int)
    };

    auto size = process_vm_writev(dst_pid, &local, 1, &remote, 1, 0);
    if (size == -1) {
        return (tl::make_unexpected(errno));
    }

    return {};
}

}
}
}
