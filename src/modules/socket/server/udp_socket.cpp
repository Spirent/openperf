#include <optional>
#include <arpa/inet.h>
#include <sys/uio.h>

#include "socket/server/udp_socket.h"
#include "lwip/udp.h"

namespace icp {
namespace socket {
namespace server {

udp_socket::udp_socket()
    : m_pcb(udp_new())
{
    if (!m_pcb) {
        throw std::runtime_error("Out of UDP pcb's!");
    }
}

udp_socket::~udp_socket()
{
    udp_remove(m_pcb);
}

static std::optional<ip_addr_t> get_address(const sockaddr_storage& sstorage)
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

static std::optional<in_port_t> get_port(const sockaddr_storage& sstorage)
{
    switch (sstorage.ss_family) {
    case AF_INET: {
        auto sa4 = reinterpret_cast<const sockaddr_in*>(&sstorage);
        return (std::make_optional(sa4->sin_port));
    }
    case AF_INET6: {
        auto sa6 = reinterpret_cast<const sockaddr_in6*>(&sstorage);
        return (std::make_optional(sa6->sin6_port));
    }
    case AF_UNSPEC:
        return (std::make_optional(0));
    default:
        return (std::nullopt);
    }
}

static tl::expected<void, int> copy_in(pid_t pid,
                                       const sockaddr* sa, socklen_t sa_len,
                                       struct sockaddr_storage& sstorage)
{
    constexpr socklen_t slength = sizeof(sstorage);
    if (sa_len > slength) {
        return (tl::make_unexpected(ENAMETOOLONG));
    }

    auto local = iovec{
        .iov_base = &sstorage,
        .iov_len = slength
    };

    auto remote = iovec{
        .iov_base = const_cast<sockaddr*>(sa),
        .iov_len = sa_len
    };

    auto size = process_vm_readv(pid, &local, 1, &remote, 1, 0);
    if (size == -1) {
        return (tl::make_unexpected(errno));
    }

    return {};
}

udp_socket::on_request_reply udp_socket::on_request(udp_init&,
                                                    const api::request_bind& bind)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(bind.id.pid, bind.name, bind.namelen, sstorage);
    if (!copy_result) {
        return {tl::make_unexpected(copy_result.error()), std::nullopt};
    }

    auto ip_addr = get_address(sstorage);
    auto ip_port = get_port(sstorage);

    if (!ip_addr || !ip_port) {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    auto error = udp_bind(m_pcb, &*ip_addr, *ip_port);
    if (error != ERR_OK) {
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    return {api::reply_success(), udp_bound()};
}

/* provide udp_disconnect a return type so that it's less cumbersome to use */
static err_t do_udp_disconnect(udp_pcb* pcb)
{
    udp_disconnect(pcb);
    return (ERR_OK);
}

udp_socket::on_request_reply udp_socket::on_request(udp_init&,
                                                    const api::request_connect& connect)
{
    sockaddr_storage sstorage;

    auto copy_result = copy_in(connect.id.pid, connect.name, connect.namelen, sstorage);
    if (!copy_result) {
        return {tl::make_unexpected(copy_result.error()), std::nullopt};
    }

    auto ip_addr = get_address(sstorage);
    auto ip_port = get_port(sstorage);

    if (!ip_addr || !ip_port) {
        return {tl::make_unexpected(EINVAL), std::nullopt};
    }

    /* connection-less sockets allow both connects and disconnects */
    auto error = (IP_IS_ANY_TYPE_VAL(*ip_addr)
                  ? do_udp_disconnect(m_pcb)
                  : udp_connect(m_pcb, &*ip_addr, *ip_port));
    if (error != ERR_OK) {
        return {tl::make_unexpected(err_to_errno(error)), std::nullopt};
    }

    return {api::reply_success(), udp_connected()};
}

}
}
}
