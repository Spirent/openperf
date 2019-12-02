#include <cassert>
#include <cstring>
#include <numeric>
#include <optional>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include "socket/server/icmp_socket.hpp"
#include "socket/server/lwip_utils.hpp"
#include "socket/server/raw_socket.hpp"
#include "lwip/icmp.h"
#include "lwip/memp.h"
#include "lwip/raw.h"

namespace openperf {
namespace socket {
namespace server {

/*
 * XXX: Uses LwIP functions that reference global variables instead of parsing
 * the pbuf, however this signature is used in case that ever needs to change.
 */
static uint8_t icmp_type(const pbuf*)
{
    return (ip_current_is_v6() ? ICMPH_TYPE(
                static_cast<const icmp6_hdr*>(ip_next_header_ptr()))
                               : ICMPH_TYPE(static_cast<const icmp_echo_hdr*>(
                                   ip_next_header_ptr())));
}

/* RAW receive function; payload should point to IP header */
static uint8_t
icmp_receive_raw(void* arg, raw_pcb*, pbuf* p, const ip_addr_t* addr)
{
    assert(addr != nullptr);
    auto sock = reinterpret_cast<icmp_socket*>(arg);

    if (sock->is_filtered(icmp_type(p))) {
        pbuf_free(p);
        return (1); /* pbuf was consumed */
    }

    auto channel = std::get<dgram_channel*>(sock->channel());
    return (channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), 0));
}

/* DGRAM receive functions; payload should point to ICMP(v6) header */
static uint8_t
icmp_receive_dgram(void* arg, raw_pcb*, pbuf* p, const ip_addr_t* addr)
{
    assert(addr != nullptr);
    auto sock = reinterpret_cast<icmp_socket*>(arg);

    if (sock->is_filtered(icmp_type(p))) {
        pbuf_free(p);
        return (1); /* pbuf was consumed */
    }

    /*
     * Drop the IP header so that the payload points to the ICMP/ICMPv6 header.
     * XXX: ip_current_header_tot_len uses a global variable inside LwIP.
     */
    pbuf_remove_header(p, ip_current_header_tot_len());

    auto channel = std::get<dgram_channel*>(sock->channel());
    return (channel->send(p, reinterpret_cast<const dgram_ip_addr*>(addr), 0));
}

static raw_recv_fn get_receive_function(int type)
{
    switch (type) {
    case SOCK_DGRAM:
        return (icmp_receive_dgram);
    case SOCK_RAW:
        return (icmp_receive_raw);
    default:
        throw std::runtime_error("Invalid icmp socket type: "
                                 + std::to_string(type));
    }
}

icmp_socket::icmp_socket(openperf::socket::server::allocator& allocator,
                         int flags,
                         int protocol)
    : raw_socket(allocator, flags, protocol, get_receive_function(flags & 0xff))
    , m_filter(0)
{}

icmp_socket& icmp_socket::operator=(icmp_socket&& other) noexcept
{
    if (this != &other) { m_filter = other.m_filter; }
    return (*this);
}

icmp_socket::icmp_socket(icmp_socket&& other) noexcept
    : raw_socket(std::move(other))
    , m_filter(other.m_filter)
{}

tl::expected<socklen_t, int>
icmp_socket::do_getsockopt(const raw_pcb* pcb,
                           const api::request_getsockopt& getsockopt)
{
    switch (getsockopt.level) {
    case SOL_RAW:
        switch (getsockopt.optname) {
        case LINUX_ICMP_FILTER: {
            if (pcb->protocol != IPPROTO_ICMP)
                return (tl::make_unexpected(EOPNOTSUPP));
            auto filter =
                linux_icmp_filter{static_cast<uint32_t>(m_filter.to_ulong())};
            auto result =
                copy_out(getsockopt.id.pid,
                         getsockopt.optval,
                         &filter,
                         std::min(static_cast<unsigned>(sizeof(filter)),
                                  getsockopt.optlen));
            if (!result) return (tl::make_unexpected(result.error()));
            return (sizeof(filter));
        }
        default:
            return (tl::make_unexpected(ENOPROTOOPT));
        }
    case SOL_SOCKET:
        return (do_sock_getsockopt(reinterpret_cast<const ip_pcb*>(pcb),
                                   getsockopt));
    case IPPROTO_IP:
        return (
            do_ip_getsockopt(reinterpret_cast<const ip_pcb*>(pcb), getsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

tl::expected<void, int>
icmp_socket::do_setsockopt(raw_pcb* pcb,
                           const api::request_setsockopt& setsockopt)
{
    switch (setsockopt.level) {
    case SOL_RAW:
        switch (setsockopt.optname) {
        case LINUX_ICMP_FILTER: {
            if (pcb->protocol != IPPROTO_ICMP)
                return (tl::make_unexpected(EOPNOTSUPP));
            auto filter = linux_icmp_filter{0};
            auto opt = copy_in(reinterpret_cast<char*>(&filter),
                               setsockopt.id.pid,
                               reinterpret_cast<const char*>(setsockopt.optval),
                               sizeof(filter),
                               setsockopt.optlen);
            if (!opt) return (tl::make_unexpected(opt.error()));
            m_filter = icmp_filter(filter.data);
            return {};
        }
        default:
            return (tl::make_unexpected(ENOPROTOOPT));
        }
    case SOL_SOCKET:
        return (do_sock_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    case IPPROTO_IP:
        return (do_ip_setsockopt(reinterpret_cast<ip_pcb*>(pcb), setsockopt));
    default:
        return (tl::make_unexpected(ENOPROTOOPT));
    }
}

bool icmp_socket::is_filtered(uint8_t type) const
{
    return (m_filter.test(type));
}

} // namespace server
} // namespace socket
} // namespace openperf
